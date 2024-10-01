/*
 * Copyright (C) 2022 Jolla Ltd.
 * Copyright (C) 2022 Slava Monich <slava.monich@jolla.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "qti_ims.h"
#include "qti_slot.h"

#include <binder_ext_ims_impl.h>

#include <ofono/log.h>

typedef GObjectClass QtiImsClass;
typedef struct qti_ims {
    GObject parent;
    char* slot;
} QtiIms;

static
void
qti_ims_iface_init(
    BinderExtImsInterface* iface);

GType qti_ims_get_type() G_GNUC_INTERNAL;
G_DEFINE_TYPE_WITH_CODE(QtiIms, qti_ims, G_TYPE_OBJECT,
G_IMPLEMENT_INTERFACE(BINDER_EXT_TYPE_IMS, qti_ims_iface_init))

#define THIS_TYPE qti_ims_get_type()
#define THIS(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, THIS_TYPE, QtiIms)
#define PARENT_CLASS qti_ims_parent_class

enum qti_ims_signal {
    SIGNAL_STATE_CHANGED,
    SIGNAL_COUNT
};

#define SIGNAL_STATE_CHANGED_NAME    "qti-ims-state-changed"

static guint qti_ims_signals[SIGNAL_COUNT] = { 0 };

/*==========================================================================*
 * BinderExtImsInterface
 *==========================================================================*/

static
BINDER_EXT_IMS_STATE
qti_ims_get_state(
    BinderExtIms* ext)
{
    QtiIms* self = THIS(ext);

    DBG("%s", self->slot);
#pragma message("TODO: return the actual state")
    return BINDER_EXT_IMS_STATE_UNKNOWN;
}

static
guint
qti_ims_set_registration(
    BinderExtIms* ext,
    BINDER_EXT_IMS_REGISTRATION registration,
    BinderExtImsResultFunc complete,
    GDestroyNotify destroy,
    void* user_data)
{
    QtiIms* self = THIS(ext);
    const gboolean on = (registration != BINDER_EXT_IMS_REGISTRATION_OFF);

    DBG("%s %s", self->slot, on ? "on" : "off");
    if (on) {
#pragma message("TODO: turn IMS registration on")
    } else {
#pragma message("TODO: turn IMS registration off")
    }
    return 0;
}

static
void
qti_ims_cancel(
    BinderExtIms* ext,
    guint id)
{
    QtiIms* self = THIS(ext);

    /*
     * Cancel a pending operation identified by the id returned by the
     * above qti_ims_set_registration() call.
     */
    DBG("%s %u", self->slot, id);
}

static
gulong
qti_ims_add_state_handler(
    BinderExtIms* ext,
    BinderExtImsFunc handler,
    void* user_data)
{
    QtiIms* self = THIS(ext);

    DBG("%s", self->slot);
    return G_LIKELY(handler) ? g_signal_connect(self,
        SIGNAL_STATE_CHANGED_NAME, G_CALLBACK(handler), user_data) : 0;
}

static
void
qti_ims_iface_init(
    BinderExtImsInterface* iface)
{
    iface->version = BINDER_EXT_IMS_INTERFACE_VERSION;
    iface->get_state = qti_ims_get_state;
    iface->set_registration = qti_ims_set_registration;
    iface->cancel = qti_ims_cancel;
    iface->add_state_handler = qti_ims_add_state_handler;
}

/*==========================================================================*
 * API
 *==========================================================================*/

BinderExtIms*
qti_ims_new(
    const char* slot)
{
    QtiIms* self = g_object_new(THIS_TYPE, NULL);

    /*
     * This could be the place to register a listener that gets invoked
     * on registration state change and emits SIGNAL_STATE_CHANGED.
     */
    self->slot = g_strdup(slot);
    return BINDER_EXT_IMS(self);
}

/*==========================================================================*
 * Internals
 *==========================================================================*/

static
void
qti_ims_finalize(
    GObject* object)
{
    QtiIms* self = THIS(object);

    g_free(self->slot);
    G_OBJECT_CLASS(PARENT_CLASS)->finalize(object);
}

static
void
qti_ims_init(
    QtiIms* self)
{
}

static
void
qti_ims_class_init(
    QtiImsClass* klass)
{
    G_OBJECT_CLASS(klass)->finalize = qti_ims_finalize;
    qti_ims_signals[SIGNAL_STATE_CHANGED] =
        g_signal_new(SIGNAL_STATE_CHANGED_NAME, G_OBJECT_CLASS_TYPE(klass),
            G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
