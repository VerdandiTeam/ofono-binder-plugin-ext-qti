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

#ifndef QTI_IMS_CALL_H
#define QTI_IMS_CALL_H

#include <binder_ext_call.h>

typedef struct qti_radio_ext QtiRadioExt;

BinderExtCall*
qti_ims_call_new(
    QtiRadioExt* ims_radio)
    G_GNUC_INTERNAL;

#endif /* QTI_IMS_CALL_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
