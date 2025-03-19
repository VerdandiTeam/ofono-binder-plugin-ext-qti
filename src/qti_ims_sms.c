/*
 *  oFono - Open Source Telephony - binder based adaptation QTI plugin
 *
 *  Copyright (C) 2022 Jolla Ltd.
 *  Copyright (C) 2024 TheKit <thekit@disroot.org>
 *  Copyright (C) 2024 Marius Gripsgard <marius@ubports.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 */

#include <glib-object.h>

#include "qti_ims_sms.h"
#include "qti_radio_ext.h"
#include "qti_radio_ext_types.h"

#include <binder_ext_sms_impl.h>

#include <ofono/log.h>

#include <radio_client.h>
#include <radio_request.h>

#include <gbinder.h>

#include <gutil_idlepool.h>
#include <gutil_macros.h>
#include <gutil_misc.h>
#include <gutil_log.h>

#define DBG(fmt, ...) \
    gutil_log(GLOG_MODULE_CURRENT, GLOG_LEVEL_ALWAYS, "ims:"fmt, ##__VA_ARGS__)

typedef GObjectClass QtiImsSmsClass;
typedef struct qti_ims_sms {
    GObject parent;
    GUtilIdlePool* pool;
    QtiRadioExt* radio_ext;
    GPtrArray* sms;
    GHashTable* id_map;
} QtiImsSms;

typedef struct qti_ims_sms_result_request {
    int ref_count;
    guint id;
    guint id_mapped;
    guint param;
    BinderExtSms* ext;
    BinderExtSmsSendFunc complete;
    GDestroyNotify destroy;
    void* user_data;
} QtiImsSmsResultRequest;

static
void
qti_ims_sms_iface_init(
    BinderExtSmsInterface* iface);

GType qti_ims_sms_get_type() G_GNUC_INTERNAL;
G_DEFINE_TYPE_WITH_CODE(QtiImsSms, qti_ims_sms, G_TYPE_OBJECT,
G_IMPLEMENT_INTERFACE(BINDER_EXT_TYPE_SMS, qti_ims_sms_iface_init))

#define THIS_TYPE qti_ims_sms_get_type()
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, QtiImsSms)
#define PARENT_CLASS qti_ims_sms_parent_class

#define ID_KEY(id) GUINT_TO_POINTER(id)
#define ID_VALUE(id) GUINT_TO_POINTER(id)

enum qti_ims_sms_signal {
    SIGNAL_SMS_STATE_CHANGED,
    SIGNAL_SMS_RECEIVED,
    SIGNAL_COUNT
};

#define SIGNAL_SMS_STATE_CHANGED_NAME    "qti-ims-sms-state-changed"
#define SIGNAL_SMS_RECEIVED_NAME         "qti-ims-sms-received"

static guint qti_ims_sms_signals[SIGNAL_COUNT] = { 0 };

static
QtiImsSmsResultRequest*
qti_ims_sms_result_request_new(
    BinderExtSms* self,
    BinderExtSmsSendFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    QtiImsSmsResultRequest* req =
        g_slice_new0(QtiImsSmsResultRequest);

    req->ref_count = 1;
    req->ext = binder_ext_sms_ref(self);
    req->complete = complete;
    req->destroy = destroy;
    req->user_data = user_data;
    return req;
}

static
void
qti_ims_sms_result_request_free(
    QtiImsSmsResultRequest* req)
{
    BinderExtSms* ext = req->ext;

    if (req->destroy) {
        req->destroy(req->user_data);
    }
    if (req->id_mapped) {
        g_hash_table_remove(THIS(ext)->id_map, ID_KEY(req->id_mapped));
    }
    binder_ext_sms_unref(ext);
    gutil_slice_free(req);
}

static
gboolean
qti_ims_sms_result_request_unref(
    QtiImsSmsResultRequest* req)
{
    if (!--(req->ref_count)) {
        qti_ims_sms_result_request_free(req);
        return TRUE;
    } else {
        return FALSE;
    }
}

static
void
qti_ims_sms_result_request_destroy(
    gpointer req)
{
    qti_ims_sms_result_request_unref(req);
}

static
void
qti_ims_sms_result_request_response(
    QtiRadioExt* radio,
    int msg_ref,
    GBinderReader* reader,
    void* user_data)
{
    gint32 result;
    GBinderReader r;
    QtiImsSmsResultRequest* req = user_data;
    BINDER_EXT_SMS_SEND_RESULT send_result;

    gbinder_reader_copy(&r, reader);
    if (!gbinder_reader_read_int32(&r, &result)) {
        ofono_warn("Failed to parse response");
        result = QTI_RADIO_SEND_STATUS_ERROR;
    }

    DBG("qti_ims_sms_result_request_response\n");
    DBG("result: %d\n", result);

    if (result == QTI_RADIO_SEND_STATUS_OK) {
        send_result = BINDER_EXT_SMS_SEND_RESULT_OK;
    } else {
        send_result = BINDER_EXT_SMS_SEND_RESULT_ERROR;
    }

    if (req->complete) {
        req->complete(req->ext, send_result, 0, req->user_data);
    }
}

static
void
qti_ims_sms_incoming_sms_handler(
    QtiRadioExt* radio,
    const void* pdu,
    guint pdu_len,
    void* user_data)
{
    QtiImsSms* self = user_data;

    DBG("Incoming SMS!!!: pdu_len=%d", pdu_len);

    g_signal_emit(self, qti_ims_sms_signals[SIGNAL_SMS_RECEIVED], 0, pdu, pdu_len);
}


/*==========================================================================*
 * BinderExtSmsInterface
 *==========================================================================*/

static
guint
qti_ims_sms_send(
    BinderExtSms* ext,
    const char* smsc,
    const void* pdu,
    gsize pdu_len,
    guint msg_ref,
    BINDER_EXT_SMS_SEND_FLAGS flags,
    BinderExtSmsSendFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    QtiImsSms* self = THIS(ext);
    QtiImsSmsResultRequest* req = qti_ims_sms_result_request_new(ext,
        complete, destroy, user_data);

    guint id = qti_radio_ext_send_ims_sms(self->radio_ext, smsc, pdu, pdu_len, msg_ref, flags,
        qti_ims_sms_result_request_response, qti_ims_sms_result_request_destroy, req);

    DBG("Sending SMS: pdu_len=%u, msg_ref=%u", pdu_len, msg_ref);

    if (id) {
        req->id = id;
        g_hash_table_insert(self->id_map, ID_KEY(id), ID_VALUE(id));
    } else {
        qti_ims_sms_result_request_free(req);
    }

    return id;
}

static
void
qti_ims_sms_cancel(
    BinderExtSms* ext,
    guint id)
{
    QtiImsSms* self = THIS(ext);
    const guint mapped = GPOINTER_TO_UINT(g_hash_table_lookup(self->id_map,
        ID_KEY(id)));

    qti_radio_ext_cancel(self->radio_ext, mapped ? mapped : id);
}

static
void
qti_ims_sms_ack_report(
    BinderExtSms* ext,
    guint msg_ref,
    gboolean ok)
{
    QtiImsSms* self = THIS(ext);

    QtiImsSmsResultRequest* req = qti_ims_sms_result_request_new(ext,
        NULL, NULL, NULL);

    guint id = qti_radio_ext_acknowledge_sms(self->radio_ext, msg_ref, ok,
        qti_ims_sms_result_request_response, qti_ims_sms_result_request_destroy, req);

    DBG("Acknowledging SMS report: msg_ref=%u, ok=%d", msg_ref, ok);

    if (id) {
        req->id = id;
        g_hash_table_insert(self->id_map, ID_KEY(id), ID_VALUE(id));
    } else {
        qti_ims_sms_result_request_free(req);
    }
}

static
void
qti_ims_sms_ack_incoming(
    BinderExtSms* ext,
    gboolean ok)
{
    QtiImsSms* self = THIS(ext);

    QtiImsSmsResultRequest* req = qti_ims_sms_result_request_new(ext,
        NULL, NULL, NULL);

    // We *should* have message reference, but we don't
    // so we use -1, we have to change upstream to fix this
    // TODO: fix this
    guint id = qti_radio_ext_acknowledge_sms(self->radio_ext, -1, ok,
        qti_ims_sms_result_request_response, qti_ims_sms_result_request_destroy, req);

    DBG("Acknowledging incoming SMS: ok=%d", ok);

    if (id) {
        req->id = id;
        g_hash_table_insert(self->id_map, ID_KEY(id), ID_VALUE(id));
    } else {
        qti_ims_sms_result_request_free(req);
    }
}

static
gulong
qti_ims_sms_add_report_handler(
    BinderExtSms* ext,
    BinderExtSmsReportFunc handler,
    void* user_data)
{
    return g_signal_connect(ext, SIGNAL_SMS_STATE_CHANGED_NAME, G_CALLBACK(handler), user_data);
}

static
gulong
qti_ims_sms_add_incoming_handler(
    BinderExtSms* ext,
    BinderExtSmsIncomingFunc handler,
    void* user_data)
{
    return g_signal_connect(ext, SIGNAL_SMS_RECEIVED_NAME, G_CALLBACK(handler), user_data);
}

static
void
qti_ims_sms_remove_handler(
    BinderExtSms* ext,
    gulong id)
{
    g_signal_handler_disconnect(ext, id);
}

void
qti_ims_sms_iface_init(
    BinderExtSmsInterface* iface)
{
    iface->flags |= BINDER_EXT_SMS_INTERFACE_FLAG_IMS_SUPPORT |
        BINDER_EXT_SMS_INTERFACE_FLAG_IMS_REQUIRED;
    iface->version = BINDER_EXT_SMS_INTERFACE_VERSION;
    iface->send = qti_ims_sms_send;
    iface->cancel = qti_ims_sms_cancel;
    iface->ack_report = qti_ims_sms_ack_report;
    iface->ack_incoming = qti_ims_sms_ack_incoming;
    iface->add_report_handler = qti_ims_sms_add_report_handler;
    iface->add_incoming_handler = qti_ims_sms_add_incoming_handler;
    iface->remove_handler = qti_ims_sms_remove_handler;
}

/*==========================================================================*
 * API
 *==========================================================================*/

BinderExtSms*
qti_ims_sms_new(
    QtiRadioExt* radio_ext)
{
    if (G_LIKELY(radio_ext)) {
        QtiImsSms* self = g_object_new(THIS_TYPE, NULL);

        self->radio_ext = qti_radio_ext_ref(radio_ext);
        self->sms = g_ptr_array_new_with_free_func(g_free);

        qti_radio_ext_add_incoming_sms_handler(radio_ext, qti_ims_sms_incoming_sms_handler, self);

        return BINDER_EXT_SMS(self);
    }
    return NULL;
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
qti_ims_sms_finalize(
    GObject* object)
{
    QtiImsSms* self = THIS(object);
    qti_radio_ext_unref(self->radio_ext);
    gutil_idle_pool_destroy(self->pool);
    g_ptr_array_free(self->sms, TRUE);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
qti_ims_sms_init(
    QtiImsSms* self)
{
    self->pool = gutil_idle_pool_new();
}

static
void
qti_ims_sms_class_init(
    QtiImsSmsClass* klass)
{
    GType type = G_OBJECT_CLASS_TYPE(klass);

    G_OBJECT_CLASS(klass)->finalize = qti_ims_sms_finalize;
    qti_ims_sms_signals[SIGNAL_SMS_STATE_CHANGED] =
        g_signal_new(SIGNAL_SMS_STATE_CHANGED_NAME, type,
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
    qti_ims_sms_signals[SIGNAL_SMS_RECEIVED] =
        g_signal_new(SIGNAL_SMS_RECEIVED_NAME, type,
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
            2, G_TYPE_POINTER, G_TYPE_UINT);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
