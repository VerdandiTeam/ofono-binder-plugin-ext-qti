/*
 *  oFono - Open Source Telephony - binder based adaptation QTI plugin
 *
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

#ifndef QTI_RADIO_EXT_H
#define QTI_RADIO_EXT_H

#include <radio_types.h>
#include <binder_ext_ims_impl.h>

#include "qti_radio_ext_types.h"


typedef struct qti_radio_ext QtiRadioExt;

typedef void (*QtiRadioExtResultFunc)(
    QtiRadioExt* radio,
    int result,
    void* user_data);

typedef void (*QtiRadioExtImsRegStatusFunc)(
    QtiRadioExt* radio,
    guint32 status,
    void* user_data);

QtiRadioExt*
qti_radio_ext_new(
    const char* dev,
    const char* slot);

QtiRadioExt*
qti_radio_ext_ref(
    QtiRadioExt* self);

void
qti_radio_ext_unref(
    QtiRadioExt* self);

guint
qti_radio_ext_set_reg_state(
    QtiRadioExt* self,
    BINDER_EXT_IMS_REGISTRATION state,
    QtiRadioExtResultFunc complete,
    GDestroyNotify destroy,
    void* user_data);

gulong
qti_radio_ext_add_ims_reg_status_handler(
    QtiRadioExt* self,
    QtiRadioExtImsRegStatusFunc handler,
    void* user_data);


#endif /* QTI_RADIO_EXT_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
