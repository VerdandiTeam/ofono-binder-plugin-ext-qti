/*
 *  oFono - Open Source Telephony - binder based adaptation QTI plugin
 *
 *  Copyright (C) 2022 Jolla Ltd.
 *  Copyright (C) 2024 TheKit <thekit@disroot.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include <glib-object.h>

#include "qti_radio_ext.h"
#include "qti_radio_ext_types.h"

#include <radio_types.h>
#include <binder_ext_ims_impl.h>
#include <binder_ext_call_impl.h>

#include <ofono/log.h>
#include <gbinder.h>

#include <gutil_idlepool.h>
#include <gutil_log.h>
#include <gutil_macros.h>

#define DBG(fmt, ...) \
    gutil_log(GLOG_MODULE_CURRENT, GLOG_LEVEL_ALWAYS, "ims:"fmt, ##__VA_ARGS__)


#define QTI_RADIO_CALL_TIMEOUT (3*1000) /* ms */

typedef GObjectClass QtiRadioExtClass;
typedef struct qti_radio_ext {
    GObject parent;
    char* slot;
    GBinderClient* client;
    GBinderRemoteObject* remote;
    GBinderLocalObject* response;
    GBinderLocalObject* indication;
    GUtilIdlePool* pool;
    GHashTable* requests;
} QtiRadioExt;

GType qti_radio_ext_get_type() G_GNUC_INTERNAL;
G_DEFINE_TYPE(QtiRadioExt, qti_radio_ext, G_TYPE_OBJECT)

#define THIS_TYPE qti_radio_ext_get_type()
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, QtiRadioExt)
#define PARENT_CLASS qti_radio_ext_parent_class
#define KEY(serial) GUINT_TO_POINTER(serial)

typedef struct qti_radio_ext_request QtiRadioExtRequest;

typedef void (*QtiRadioExtArgWriteFunc)(
    GBinderWriter* args,
    va_list va);

typedef void (*QtiRadioExtRequestHandlerFunc)(
    QtiRadioExtRequest* req,
    const GBinderReader* args);

typedef void (*QtiRadioExtResultFunc)(
    QtiRadioExt* radio,
    int result,
    GBinderReader* reader,
    void* user_data);

struct qti_radio_ext_request {
    guint id;  /* request id */
    gulong tx; /* binder transaction id */
    QtiRadioExt* radio;
    gint32 response_code;
    QtiRadioExtRequestHandlerFunc handle_response;
    void (*free)(QtiRadioExtRequest* req);
    GDestroyNotify destroy;
    void* user_data;
};

typedef struct qti_radio_ext_result_request {
    QtiRadioExtRequest base;
    QtiRadioExtResultFunc complete;
} QtiRadioExtResultRequest;

enum qti_radio_ext_signal {
    SIGNAL_IMS_REG_STATUS_CHANGED,
    SIGNAL_EXT_CALL_STATE_CHANGED,
    SIGNAL_COUNT
};

#define SIGNAL_IMS_REG_STATUS_CHANGED_NAME      "qti-radio-ext-ims-reg-status-changed"
#define SIGNAL_EXT_CALL_STATE_CHANGED_NAME          "qti-radio-ext-call-state-changed"

static guint qti_radio_ext_signals[SIGNAL_COUNT] = { 0 };

static GLogModule qti_radio_ext_binder_log_module = {
    .max_level = GLOG_LEVEL_VERBOSE,
    .level = GLOG_LEVEL_VERBOSE,
    .flags = GLOG_FLAG_HIDE_NAME
};

static GLogModule qti_radio_ext_binder_dump_module = {
    .parent = &qti_radio_ext_binder_log_module,
    .max_level = GLOG_LEVEL_VERBOSE,
    .level = GLOG_LEVEL_INHERIT,
    .flags = GLOG_FLAG_HIDE_NAME
};

static
void
qti_radio_ext_log_notify(
    struct ofono_debug_desc* desc)
{
    qti_radio_ext_binder_log_module.level = (desc->flags &
        OFONO_DEBUG_FLAG_PRINT) ? GLOG_LEVEL_VERBOSE : GLOG_LEVEL_INHERIT;
}

static
void
qti_radio_ext_dump_notify(
    struct ofono_debug_desc* desc)
{
    qti_radio_ext_binder_dump_module.level = (desc->flags &
        OFONO_DEBUG_FLAG_PRINT) ? GLOG_LEVEL_VERBOSE : GLOG_LEVEL_INHERIT;
}

static struct ofono_debug_desc logger_trace OFONO_DEBUG_ATTR = {
    .name = "qti_binder_trace",
    .flags = OFONO_DEBUG_FLAG_DEFAULT | OFONO_DEBUG_FLAG_HIDE_NAME,
    .notify = qti_radio_ext_log_notify
};

static struct ofono_debug_desc logger_dump OFONO_DEBUG_ATTR = {
    .name = "qti_binder_dump",
    .flags = OFONO_DEBUG_FLAG_DEFAULT | OFONO_DEBUG_FLAG_HIDE_NAME,
    .notify = qti_radio_ext_dump_notify
};

static
guint
qti_radio_ext_new_req_id()
{
    static guint last_id = 0;
    return last_id++;
}

static const char*
qti_radio_ext_req_name(
    guint32 req)
{
    switch (req) {
#define QTI_RADIO_REQ_(req, resp, name, NAME) \
        case QTI_RADIO_REQ_##NAME: return #name;
    QTI_RADIO_EXT_IMS_CALL_1_0(QTI_RADIO_REQ_)
    QTI_RADIO_EXT_IMS_CALL_1_1(QTI_RADIO_REQ_)
    QTI_RADIO_EXT_IMS_CALL_1_2(QTI_RADIO_REQ_)
#undef QTI_RADIO_REQ_
    }
    return NULL;
}

static const char*
qti_radio_ext_resp_name(
    guint32 resp)
{
    switch (resp) {
#define QTI_RADIO_RESP_(req, resp, name, NAME) \
        case QTI_RADIO_RESP_##NAME: return #name;
    QTI_RADIO_EXT_IMS_CALL_1_0(QTI_RADIO_RESP_)
    QTI_RADIO_EXT_IMS_CALL_1_1(QTI_RADIO_RESP_)
    QTI_RADIO_EXT_IMS_CALL_1_2(QTI_RADIO_RESP_)
#undef QTI_RADIO_RESP_
    }
    return NULL;
}

static const char*
qti_radio_ext_ind_name(
    guint32 ind)
{
    switch (ind) {
#define QTI_RADIO_IND_(code, name, NAME) \
        case QTI_RADIO_IND_##NAME: return #name;
    QTI_RADIO_IND_1_0(QTI_RADIO_IND_)
#undef QTI_RADIO_IND_
    }
    return NULL;
}

static
void
qti_radio_ext_log_req(
    QtiRadioExt* self,
    guint32 code,
    guint32 serial)
{
    static const GLogModule* log = &qti_radio_ext_binder_log_module;
    const int level = GLOG_LEVEL_VERBOSE;
    const char* name;

    if (!gutil_log_enabled(log, level))
        return;

    name = qti_radio_ext_req_name(code);

    if (serial) {
        gutil_log(log, level, "%s< [%08x] %u %s",
            self->slot, serial, code, name ? name : "???");
    } else {
        gutil_log(log, level, "%s< %u %s",
            self->slot, code, name ? name : "???");
    }
}

void
qti_radio_ext_log_resp(
    QtiRadioExt* self,
    guint32 code,
    guint32 serial)
{
    static const GLogModule* log = &qti_radio_ext_binder_log_module;
    const int level = GLOG_LEVEL_VERBOSE;
    const char* name;

    if (!gutil_log_enabled(log, level))
        return;

    name = qti_radio_ext_resp_name(code);

    gutil_log(log, level, "%s> [%08x] %u %s",
        self->slot, serial, code, name ? name : "???");
}

static
void
qti_radio_ext_log_ind(
    QtiRadioExt* self,
    guint32 code)
{
    static const GLogModule* log = &qti_radio_ext_binder_log_module;
    const int level = GLOG_LEVEL_VERBOSE;
    const char* name;

    if (!gutil_log_enabled(log, level))
        return;

    name = qti_radio_ext_ind_name(code);

    gutil_log(log, level, "%s > %u %s", self->slot, code,
        name ? name : "???");
}

static
void
qti_radio_ext_dump_data(
    const GBinderReader* reader)
{
    static const GLogModule* log = &qti_radio_ext_binder_dump_module;
    const int level = GLOG_LEVEL_VERBOSE;
    gsize size;
    const guint8* data;

    if (!gutil_log_enabled(log, level))
        return;

    data = gbinder_reader_get_data(reader, &size);
    gutil_log_dump(log, level, "  ",  data, size);
}

static
void
qti_radio_ext_dump_request(
    GBinderLocalRequest* args)
{
    static const GLogModule* log = &qti_radio_ext_binder_dump_module;
    const int level = GLOG_LEVEL_VERBOSE;
    GBinderWriter writer;
    const guint8* data;
    gsize size;

    if (!gutil_log_enabled(log, level))
        return;

    /* Use writer API to fetch the raw data */
    gbinder_local_request_init_writer(args, &writer);
    data = gbinder_writer_get_data(&writer, &size);
    gutil_log_dump(log, level, "  ", data, size);
}

const QtiRadioRegInfo*
qti_radio_ext_read_ims_reg_status_info(
    QtiRadioExt* self,
    GBinderReader* reader)
{
    const QtiRadioRegInfo* info;

    info = gbinder_reader_read_hidl_struct(reader, QtiRadioRegInfo);
    if (info) {
        const char *uri = info->uri.data.str ? info->uri.data.str : "";
        const char *error_msg = info->error_message.data.str ? info->error_message.data.str : "";

        DBG("%s: QtiRadioRegInfo state:%d radiotech:%d"
            " error_code:%d\n"
            " uri:%s error_msg:%s",
            self->slot,
            info->state,
            info->radio_tech,
            info->error_code,
            uri, error_msg);

        return info;
    } else {
        DBG("%s: failed to parse QtiRadioRegInfo", self->slot);
        return NULL;
    }
}

static
void
qti_radio_ext_handle_ims_reg_status_report(
    QtiRadioExt* self,
    const GBinderReader* args)
{
    GBinderReader reader;

    gbinder_reader_copy(&reader, args);
    const QtiRadioRegInfo* info =
        qti_radio_ext_read_ims_reg_status_info(self, &reader);

    g_signal_emit(self, qti_radio_ext_signals[SIGNAL_IMS_REG_STATUS_CHANGED],
                    0, info->state);
}

static
BINDER_EXT_CALL_STATE
qti_ims_call_radio_state_to_state(
    QTI_RADIO_CALL_STATE state)
{
    switch (state) {
    case QTI_RADIO_CALL_STATE_INCOMING:
        return BINDER_EXT_CALL_STATE_INCOMING;
    case QTI_RADIO_CALL_STATE_ALERTING:
        return BINDER_EXT_CALL_STATE_ALERTING;
    case QTI_RADIO_CALL_STATE_HOLDING:
        return BINDER_EXT_CALL_STATE_HOLDING;
    case QTI_RADIO_CALL_STATE_WAITING:
        return BINDER_EXT_CALL_STATE_WAITING;
    case QTI_RADIO_CALL_STATE_ACTIVE:
        return BINDER_EXT_CALL_STATE_ACTIVE;
    case QTI_RADIO_CALL_STATE_END:
        return BINDER_EXT_CALL_STATE_END;
    case QTI_RADIO_CALL_STATE_DIALING:
        return BINDER_EXT_CALL_STATE_DIALING;
    default:
        return BINDER_EXT_CALL_STATE_INVALID;
    }
}

static
GPtrArray*
qti_radio_ext_read_call_state_info(
    QtiRadioCallInfo* call_info_array,
    gsize count)
{
    // array of QtiRadioCallInfo
    GPtrArray* call_ext_info_array = g_ptr_array_new_with_free_func(g_free);

    for (gsize i = 0; i < count; i++) {
        QtiRadioCallInfo* call_info = &call_info_array[i];
        
        const gsize total = G_ALIGN8(sizeof(BinderExtCallInfo)) +
            G_ALIGN8(call_info->number.len + 1) + G_ALIGN8(call_info->name.len + 1);
        BinderExtCallInfo* dest = g_malloc0(total);

        char* ptr_number = ((char*)dest) + G_ALIGN8(sizeof(BinderExtCallInfo));
        char* ptr_name = ptr_number + G_ALIGN8(call_info->number.len + 1);

        dest->call_id = call_info->index;
        dest->state = qti_ims_call_radio_state_to_state(call_info->state);
        dest->type = BINDER_EXT_CALL_TYPE_VOICE;
        dest->flags = BINDER_EXT_CALL_FLAG_IMS | BINDER_EXT_CALL_FLAG_INCOMING;

        dest->number = ptr_number;
        dest->name = ptr_name;

        memcpy(ptr_name, call_info->name.data.str, call_info->name.len);
        ptr_name += G_ALIGN8(call_info->name.len + 1);

        memcpy(ptr_number, call_info->number.data.str, call_info->number.len);
        ptr_number += G_ALIGN8(call_info->number.len + 1);

        g_ptr_array_add(call_ext_info_array, dest);

        // print call_info
        const char* number = call_info->number.data.str ? call_info->number.data.str : "";
        const char* name = call_info->name.data.str ? call_info->name.data.str : "";
        DBG("callInfoIndication state:%d index:%d name:%s number:%s",
            call_info->state, call_info->index, name, number);

    }

    return call_ext_info_array;
}

static
void
qti_radio_ext_handle_call_state_indication(
    QtiRadioExt* self,
    const GBinderReader* args)
{
    /* callInfoIndication(vec<CallInfo> callList) */
    QtiRadioCallInfo* call_infos;
    GBinderReader reader;
    gsize count;

    gbinder_reader_copy(&reader, args);
    call_infos = gbinder_reader_read_hidl_type_vec(&reader, QtiRadioCallInfo, &count);

    if (call_infos) {
        GPtrArray* call_info_ptr = qti_radio_ext_read_call_state_info(call_infos, count);

        g_signal_emit(self, qti_radio_ext_signals[SIGNAL_EXT_CALL_STATE_CHANGED],
                      0, call_info_ptr);

        g_free(call_infos);
    } else {
        DBG("%s: failed to parse call state data", self->slot);
    }
}


static
GBinderLocalReply*
qti_radio_ext_indication(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    QtiRadioExt* self = THIS(user_data);
    const char* iface = gbinder_remote_request_interface(req);
    GBinderReader args;

    gbinder_remote_request_init_reader(req, &args);
    qti_radio_ext_log_ind(self, code);
    qti_radio_ext_dump_data(&args);

    if (g_str_equal(iface, QTI_RADIO_INDICATION_1_0)) {
        switch(code) {
        case QTI_RADIO_IND_REG_STATE_INDICATION:
            qti_radio_ext_handle_ims_reg_status_report(self, &args);
            return NULL;
        case QTI_RADIO_IND_CALL_STATE_INDICATION:
            qti_radio_ext_handle_call_state_indication(self, &args);
            return NULL;
        }
    }


    return NULL;
}

gulong
qti_radio_ext_add_ims_reg_status_handler(
    QtiRadioExt* self,
    QtiRadioExtImsRegStatusFunc handler,
    void* user_data)
{
    return (G_LIKELY(self) && G_LIKELY(handler)) ? g_signal_connect(self,
        SIGNAL_IMS_REG_STATUS_CHANGED_NAME, G_CALLBACK(handler), user_data) : 0;
}

gulong
qti_radio_ext_add_call_state_handler(
    QtiRadioExt* self,
    QtiRadioExtCallStateFunc handler,
    void* user_data)
{
    return (G_LIKELY(self) && G_LIKELY(handler)) ? g_signal_connect(self,
        SIGNAL_EXT_CALL_STATE_CHANGED_NAME, G_CALLBACK(handler), user_data) : 0;
}

static
GBinderLocalReply*
qti_radio_ext_response(
    GBinderLocalObject* obj,
    GBinderRemoteRequest* req,
    guint code,
    guint flags,
    int* status,
    void* user_data)
{
    QtiRadioExt* self = THIS(user_data);
    const char* iface = gbinder_remote_request_interface(req);
    GBinderReader reader;
    guint32 serial = 0;

    gbinder_remote_request_init_reader(req, &reader);

    gbinder_reader_read_uint32(&reader, &serial);
    qti_radio_ext_log_resp(self, code, serial);
    qti_radio_ext_dump_data(&reader);

    if (serial) {
        QtiRadioExtRequest* req = g_hash_table_lookup(self->requests,
            KEY(serial));

        if (req && req->response_code == code) {
            g_object_ref(self);
            if (req->handle_response) {
                req->handle_response(req, &reader);
            }
            g_hash_table_remove(self->requests, KEY(serial));
            g_object_unref(self);
        } else {
            DBG("Unexpected response %s %u", iface, code);
            *status = GBINDER_STATUS_FAILED;
        }
    }

    return NULL;
}

static
void
qti_radio_ext_result_response(
    QtiRadioExtRequest* req,
    const GBinderReader* args)
{
    gint32 result;
    GBinderReader reader;
    QtiRadioExt* self = req->radio;
    QtiRadioExtResultRequest* result_req = G_CAST(req,
        QtiRadioExtResultRequest, base);

    gbinder_reader_copy(&reader, args);
    if (!gbinder_reader_read_int32(&reader, &result)) {
        ofono_warn("Failed to parse response");
        result = -1;
    }
    if (result_req->complete) {
        result_req->complete(self, result, args, req->user_data);
    }
}

static
void
qti_radio_ext_request_default_free(
    QtiRadioExtRequest* req)
{
    if (req->destroy) {
        req->destroy(req->user_data);
    }
    g_free(req);
}

static
void
qti_radio_ext_request_destroy(
    gpointer user_data)
{
    QtiRadioExtRequest* req = user_data;

    gbinder_client_cancel(req->radio->client, req->tx);
    req->free(req);
}

static
gpointer
qti_radio_ext_request_alloc(
    QtiRadioExt* self,
    gint32 resp,
    QtiRadioExtRequestHandlerFunc handler,
    GDestroyNotify destroy,
    void* user_data,
    gsize size)
{
    QtiRadioExtRequest* req = g_malloc0(size);

    req->radio = self;
    req->response_code = resp;
    req->handle_response = handler;
    req->id = qti_radio_ext_new_req_id(self);
    req->free = qti_radio_ext_request_default_free;
    req->destroy = destroy;
    req->user_data = user_data;
    g_hash_table_insert(self->requests, KEY(req->id), req);
    return req;
}

static
QtiRadioExtResultRequest*
qti_radio_ext_result_request_new(
    QtiRadioExt* self,
    gint32 resp,
    QtiRadioExtResultFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    QtiRadioExtResultRequest* req =
        (QtiRadioExtResultRequest*)qti_radio_ext_request_alloc(self, resp,
            qti_radio_ext_result_response, destroy, user_data,
            sizeof(QtiRadioExtResultRequest));

    req->complete = complete;
    return req;
}

static
void
qti_radio_ext_request_sent(
    GBinderClient* client,
    GBinderRemoteReply* reply,
    int status,
    void* user_data)
{
    ((QtiRadioExtRequest*)user_data)->tx = 0;
}

static
gulong
qti_radio_ext_call(
    QtiRadioExt* self,
    gint32 code,
    gint32 serial,
    GBinderLocalRequest* req,
    GBinderClientReplyFunc reply,
    GDestroyNotify destroy,
    void* user_data)
{
    qti_radio_ext_log_req(self, code, serial);
    qti_radio_ext_dump_request(req);

    return gbinder_client_transact(self->client, code,
        GBINDER_TX_FLAG_ONEWAY, req, reply, destroy, user_data);
}

void
qti_radio_ext_cancel(
    QtiRadioExt* self,
    guint id)
{
    if (G_LIKELY(self) && G_LIKELY(id)) {
        g_hash_table_remove(self->requests, KEY(id));
    }
}


static
gulong
qti_radio_ext_submit_request(
    QtiRadioExtRequest* request,
    gint32 code,
    gint32 serial,
    GBinderLocalRequest* args)
{
    return (request->tx = qti_radio_ext_call(request->radio,
        code, serial, args, qti_radio_ext_request_sent, NULL, request));
}

static
guint
qti_radio_ext_result_request_submit(
    QtiRadioExt* self,
    gint32 req_code,
    gint32 resp_code,
    QtiRadioExtArgWriteFunc write_args,
    QtiRadioExtResultFunc complete,
    GDestroyNotify destroy,
    void* user_data,
    ...)
{
    if (G_LIKELY(self)) {
        GBinderLocalRequest* args;
        GBinderWriter writer;
        QtiRadioExtResultRequest* req =
            qti_radio_ext_result_request_new(self, resp_code,
                complete, destroy, user_data);
        const guint req_id = req->base.id;

        args = gbinder_client_new_request2(self->client, req_code);
        gbinder_local_request_init_writer(args, &writer);
        gbinder_writer_append_int32(&writer, req_id);
        if (write_args) {
            va_list va;

            va_start(va, user_data);
            write_args(&writer, va);
            va_end(va);
        }

        /* Submit the request */
        qti_radio_ext_submit_request(&req->base, req_code, req_id, args);
        gbinder_local_request_unref(args);
        if (req->base.tx) {
            /* Success */
            return req_id;
        }
        g_hash_table_remove(self->requests, KEY(req_id));
    }
    return 0;
}


static
QtiRadioExt*
qti_radio_ext_create(
    GBinderServiceManager* sm,
    GBinderRemoteObject* remote,
    const char* slot)
{
    QtiRadioExt* self = g_object_new(THIS_TYPE, NULL);
    const gint code = QTI_RADIO_REQ_SET_CALLBACK;
    GBinderLocalRequest* req;
    GBinderWriter writer;
    int status;

    self->slot = g_strdup(slot);
    self->client = gbinder_client_new(remote, QTI_RADIO_1_0);
    self->response = gbinder_servicemanager_new_local_object(sm,
        QTI_RADIO_RESPONSE_1_0, qti_radio_ext_response, self);
    self->indication = gbinder_servicemanager_new_local_object(sm,
        QTI_RADIO_INDICATION_1_0, qti_radio_ext_indication, self);

    req = gbinder_client_new_request2(self->client, code);
    gbinder_local_request_init_writer(req, &writer);
    gbinder_writer_append_local_object(&writer, self->response);
    gbinder_writer_append_local_object(&writer, self->indication);

    qti_radio_ext_log_req(self, code, 0 /*serial*/);
    qti_radio_ext_dump_request(req);
    gbinder_remote_reply_unref(gbinder_client_transact_sync_reply(self->client,
        code, req, &status));

    DBG("setResponseFunctions status %d", status);
    gbinder_local_request_unref(req);
    return self;
}

/*==========================================================================*
 * API
 *==========================================================================*/


QtiRadioExt*
qti_radio_ext_new(
    const char* dev,
    const char* slot)
{
    QtiRadioExt* self = NULL;

    GBinderServiceManager* sm = gbinder_servicemanager_new(dev);
    if (sm) {
        char* fqname = g_strconcat(QTI_RADIO_1_0, "/", slot, NULL);
        GBinderRemoteObject* obj = /* autoreleased */
            gbinder_servicemanager_get_service_sync(sm, fqname, NULL);

        if (obj) {
            DBG("Connected to %s", fqname);
            self = qti_radio_ext_create(sm, obj, slot);
        } else {
            DBG("Failed to get %s", fqname);
        }

        g_free(fqname);
        gbinder_servicemanager_unref(sm);
    }

    return self;
}

QtiRadioExt*
qti_radio_ext_ref(
    QtiRadioExt* self)
{
    if (G_LIKELY(self)) {
        g_object_ref(self);
    }
    return self;
}

void
qti_radio_ext_unref(
    QtiRadioExt* self)
{
    if (G_LIKELY(self)) {
        g_object_unref(self);
    }
}

static
void
qti_radio_ext_set_reg_state_args(
    GBinderWriter* args,
    va_list va)
{
    gbinder_writer_append_int32(args, va_arg(va, gint32));
}

// BINDER_EXT_IMS_REGISTRATION to QTI_RADIO_REG_STATE
static
QTI_RADIO_REG_STATE
qti_radio_ext_reg_state(
    BINDER_EXT_IMS_REGISTRATION state)
{
    switch (state) {
    case BINDER_EXT_IMS_REGISTRATION_ON:
        return QTI_RADIO_REG_STATE_REGISTERED;
    case BINDER_EXT_IMS_REGISTRATION_OFF:
        return QTI_RADIO_REG_STATE_NOT_REGISTERED;
    default:
        return QTI_RADIO_REG_STATE_INVALID;
    }
}


guint
qti_radio_ext_set_reg_state(
    QtiRadioExt* self,
    BINDER_EXT_IMS_REGISTRATION state,
    QtiRadioExtResultFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    QTI_RADIO_REG_STATE reg_state = qti_radio_ext_reg_state(state);

    DBG("Setting registration state %d", reg_state);

    return qti_radio_ext_result_request_submit(self,
        QTI_RADIO_REQ_REQ_REG_CHANGE,
        QTI_RADIO_RESP_REQ_REG_CHANGE,
        qti_radio_ext_set_reg_state_args,
        complete, destroy, user_data,
        reg_state);
}

/* From ofono-binder-plugin's binder-util.c */
static
void
binder_copy_hidl_string(
    GBinderWriter* writer,
    GBinderHidlString* dest,
    const char* src)
{
    gssize len = src ? strlen(src) : 0;
    dest->owns_buffer = TRUE;
    if (len > 0) {
        /* GBinderWriter takes ownership of the string contents */
        dest->len = (guint32) len;
        dest->data.str = gbinder_writer_memdup(writer, src, len + 1);
    } else {
        /* Replace NULL strings with empty strings */
        dest->data.str = "";
        dest->len = 0;
    }
}

static
void
binder_append_hidl_string_with_parent(
    GBinderWriter* writer,
    const GBinderHidlString* str,
    guint32 index,
    guint32 offset)
{
    GBinderParent parent;

    parent.index = index;
    parent.offset = offset;

    /* Strings are NULL-terminated, hence len + 1 */
    gbinder_writer_append_buffer_object_with_parent(writer, str->data.str,
        str->len + 1, &parent);
}

#define binder_append_hidl_string_data(writer,ptr,field,index) \
    binder_append_hidl_string_with_parent(writer, &ptr->field, index, \
        ((guint8*)(&ptr->field) - (guint8*)ptr))

static
void
binder_append_hidl_vec_with_parent(
    GBinderWriter* writer,
    const GBinderHidlVec* vec,
    guint32 index,
    guint32 offset)
{
    GBinderParent parent;

    parent.index = index;
    parent.offset = offset;

    gbinder_writer_append_buffer_object_with_parent(writer, vec->data.ptr,
        vec->count, &parent);
}

#define binder_append_hidl_vec_data(writer,ptr,field,index) \
    binder_append_hidl_vec_with_parent(writer, &ptr->field, index, \
        ((guint8*)(&ptr->field) - (guint8*)ptr))

#define CLIR_DEFAULT 0          // "use subscription default value"
#define CLIR_INVOCATION 1       // (restrict CLI presentation)
#define CLIR_SUPPRESSION 2    // (allow CLI presentation)

static
void
qti_radio_ext_dial_args(
    GBinderWriter* args,
    va_list va)
{
    QtiRadioDialRequest* dial_request_writer;

    const char* number = va_arg(va, const char*);
    gint32 clir = va_arg(va, gint32);

    static const GBinderWriterField qti_radio_dial_request_f[] = {
        GBINDER_WRITER_FIELD_HIDL_STRING
            (QtiRadioDialRequest, address),
        GBINDER_WRITER_FIELD_HIDL_VEC_BYTE
            (QtiRadioDialRequest, call_details.extras),
        GBINDER_WRITER_FIELD_HIDL_VEC_BYTE
            (QtiRadioDialRequest, call_details.local_ability),
        GBINDER_WRITER_FIELD_HIDL_VEC_BYTE
            (QtiRadioDialRequest, call_details.peer_ability),
        GBINDER_WRITER_FIELD_HIDL_STRING
            (QtiRadioDialRequest, call_details.sip_alternate_uri),
        GBINDER_WRITER_FIELD_END()
    };
    static const GBinderWriterType qti_radio_dial_request_t = {
        GBINDER_WRITER_STRUCT_NAME_AND_SIZE(QtiRadioDialRequest),
        qti_radio_dial_request_f
    };

    dial_request_writer = gbinder_writer_new0(args, QtiRadioDialRequest);

    GBinderHidlVec* empty_vec1 = gbinder_writer_new0(args, GBinderHidlVec);
    GBinderHidlVec* empty_vec2 = gbinder_writer_new0(args, GBinderHidlVec);
    GBinderHidlVec* empty_vec3 = gbinder_writer_new0(args, GBinderHidlVec);

    dial_request_writer->clir_mode = clir;
    switch (clir)
    {
    case CLIR_SUPPRESSION:
        dial_request_writer->presentation = QTI_RADIO_IP_PRESENTATION_NUM_RESTRICTED;
        break;
    default:
        dial_request_writer->presentation = QTI_RADIO_IP_PRESENTATION_NUM_ALLOWED;
        break;
    }
    dial_request_writer->call_details.call_type = QTI_RADIO_CALL_TYPE_VOICE;
    dial_request_writer->call_details.call_domain = QTI_RADIO_CALL_DOMAIN_CS;
    dial_request_writer->call_details.extras_length = 0;
    dial_request_writer->call_details.extras.count = 0;
    dial_request_writer->call_details.extras.data.ptr = empty_vec1;
    dial_request_writer->call_details.extras.owns_buffer = TRUE;
    dial_request_writer->call_details.local_ability.count = 0;
    dial_request_writer->call_details.local_ability.data.ptr = empty_vec2;
    dial_request_writer->call_details.local_ability.owns_buffer = TRUE;
    dial_request_writer->call_details.peer_ability.count = 0;
    dial_request_writer->call_details.peer_ability.data.ptr = empty_vec3;
    dial_request_writer->call_details.peer_ability.owns_buffer = TRUE;

    dial_request_writer->call_details.call_substate = 0; // none
    dial_request_writer->call_details.media_id = -1; // unknown
    dial_request_writer->call_details.cause_code = 0; // none
    dial_request_writer->call_details.rtt_mode = 0;

    binder_copy_hidl_string(args, &dial_request_writer->address, number);
    binder_copy_hidl_string(args, &dial_request_writer->call_details.sip_alternate_uri, NULL);

    gbinder_writer_append_struct(args, dial_request_writer,
            &qti_radio_dial_request_t, NULL);

    DBG("Dialing in args New %s", number);
}

                /*
// dail
static
void
qti_radio_ext_dial_args(
    GBinderWriter* args,
    va_list va)
{
    GBinderParent parent;
    QtiRadioDialRequest* dial_request_writer;

    const char* number = va_arg(va, const char*);
    gint32 clir = va_arg(va, gint32);

    dial_request_writer = gbinder_writer_new0(args, QtiRadioDialRequest);

    GBinderHidlVec* empty_vec1 = gbinder_writer_new0(args, GBinderHidlVec);
    GBinderHidlVec* empty_vec2 = gbinder_writer_new0(args, GBinderHidlVec);
    GBinderHidlVec* empty_vec3 = gbinder_writer_new0(args, GBinderHidlVec);

    dial_request_writer->clir_mode = clir;
    switch (clir)
    {
    case CLIR_SUPPRESSION:
        dial_request_writer->presentation = QTI_RADIO_IP_PRESENTATION_NUM_RESTRICTED;
        break;
    default:
        dial_request_writer->presentation = QTI_RADIO_IP_PRESENTATION_NUM_ALLOWED;
        break;
    }
    dial_request_writer->call_details.call_type = QTI_RADIO_CALL_TYPE_VOICE;
    dial_request_writer->call_details.call_domain = QTI_RADIO_CALL_DOMAIN_CS;
    dial_request_writer->call_details.extras_length = 0;
    dial_request_writer->call_details.extras.count = 0;
    dial_request_writer->call_details.extras.data.ptr = empty_vec1;
    dial_request_writer->call_details.extras.owns_buffer = TRUE;
    dial_request_writer->call_details.local_ability.count = 0;
    dial_request_writer->call_details.local_ability.data.ptr = empty_vec2;
    dial_request_writer->call_details.local_ability.owns_buffer = TRUE;
    dial_request_writer->call_details.peer_ability.count = 0;
    dial_request_writer->call_details.peer_ability.data.ptr = empty_vec3;
    dial_request_writer->call_details.peer_ability.owns_buffer = TRUE;

    dial_request_writer->call_details.call_substate = 0; // none
    dial_request_writer->call_details.media_id = -1; // unknown
    dial_request_writer->call_details.cause_code = 0; // none
    dial_request_writer->call_details.rtt_mode = 0;

    binder_copy_hidl_string(args, &dial_request_writer->address, number);
    binder_copy_hidl_string(args, &dial_request_writer->call_details.sip_alternate_uri, NULL);

    /* Write the parent structure 
    parent.index = gbinder_writer_append_buffer_object(args, dial_request_writer,
        sizeof(*dial_request_writer));

    /* Write the string data 
    binder_append_hidl_string_data(args, dial_request_writer, address, parent.index);

    // find right index after 
    guint32 index = G_STRUCT_OFFSET(QtiRadioDialRequest, call_details.call_type);
    binder_append_hidl_string_data(args, dial_request_writer, call_details.sip_alternate_uri, parent.index);
    binder_append_hidl_vec_data(args, dial_request_writer, call_details.extras, parent.index);

    /* UUS information is empty but we still need to write a buffer 
    //parent.offset = G_STRUCT_OFFSET(QtiRadioDialRequest, call_details.extras.data.ptr);
    //gbinder_writer_append_buffer_object_with_parent(args, NULL, 0, &parent);

    //parent.offset = G_STRUCT_OFFSET(QtiRadioDialRequest, call_details.local_ability.data.ptr);
    //gbinder_writer_append_buffer_object_with_parent(args, NULL, 0, &parent);

    //parent.offset = G_STRUCT_OFFSET(QtiRadioDialRequest, call_details.peer_ability.data.ptr);
    //gbinder_writer_append_buffer_object_with_parent(args, NULL, 0, &parent);

    DBG("Dialing in args YAY %s", number);
}
*/

guint
qti_radio_ext_dial(
    QtiRadioExt* self,
    const char* number,
    BINDER_EXT_TOA toa,
    BINDER_EXT_CALL_CLIR clir,
    BINDER_EXT_CALL_DIAL_FLAGS flags,
    QtiRadioExtResultFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    return qti_radio_ext_result_request_submit(self,
        QTI_RADIO_REQ_DAIL,
        QTI_RADIO_RESP_DAIL,
        qti_radio_ext_dial_args,
        complete, destroy, user_data,
        number, clir);
}

// GET_IMS_REG_STATE

static
void
qti_radio_ext_get_ims_reg_state_args(
    GBinderWriter* args,
    va_list va)
{
    // empty
}

guint
qti_radio_ext_get_ims_reg_state(
    QtiRadioExt* self,
    QtiRadioExtResultFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    return qti_radio_ext_result_request_submit(self,
        QTI_RADIO_REQ_GET_IMS_REG_STATE,
        QTI_RADIO_RESP_GET_IMS_REG_STATE,
        qti_radio_ext_get_ims_reg_state_args,
        complete, destroy, user_data);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
qti_radio_ext_finalize(
    GObject* object)
{
    QtiRadioExt* self = THIS(object);

    g_free(self->slot);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
qti_radio_ext_init(
    QtiRadioExt* self)
{
    self->pool = gutil_idle_pool_new();
    self->requests = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
        qti_radio_ext_request_destroy);
}

static
void
qti_radio_ext_class_init(
    QtiRadioExtClass* klass)
{
    G_OBJECT_CLASS(klass)->finalize = qti_radio_ext_finalize;
    qti_radio_ext_signals[SIGNAL_IMS_REG_STATUS_CHANGED] =
        g_signal_new(SIGNAL_IMS_REG_STATUS_CHANGED_NAME, G_OBJECT_CLASS_TYPE(klass),
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
            1, G_TYPE_UINT);
    qti_radio_ext_signals[SIGNAL_EXT_CALL_STATE_CHANGED] =
        g_signal_new(SIGNAL_EXT_CALL_STATE_CHANGED_NAME, G_OBJECT_CLASS_TYPE(klass),
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE,
            1, G_TYPE_PTR_ARRAY);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
