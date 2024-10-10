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

#ifndef QTI_RADIO_EXT_TYPES_H
#define QTI_RADIO_EXT_TYPES_H

#define QTI_RADIO_IFACE                 "IImsRadio"
#define QTI_RADIO_RESPONSE_IFACE        "IImsRadioResponse"
#define QTI_RADIO_INDICATION_IFACE      "IImsRadioIndication"
#define QTI_RADIO_IFACE_PREFIX          "vendor.qti.hardware.radio.ims@"

#define QTI_RADIO_IFACE_1_0(x)          QTI_RADIO_IFACE_PREFIX "1.0::" x
#define QTI_RADIO_IFACE_1_1(x)          QTI_RADIO_IFACE_PREFIX "1.1::" x
#define QTI_RADIO_IFACE_1_2(x)          QTI_RADIO_IFACE_PREFIX "1.2::" x

#define QTI_RADIO_1_0                   QTI_RADIO_IFACE_1_0(QTI_RADIO_IFACE)
#define QTI_RADIO_1_1                   QTI_RADIO_IFACE_1_1(QTI_RADIO_IFACE)
#define QTI_RADIO_1_2                   QTI_RADIO_IFACE_1_2(QTI_RADIO_IFACE)

#define QTI_RADIO_RESPONSE_1_0          QTI_RADIO_IFACE_1_0(QTI_RADIO_RESPONSE_IFACE)
#define QTI_RADIO_RESPONSE_1_1          QTI_RADIO_IFACE_1_1(QTI_RADIO_RESPONSE_IFACE)
#define QTI_RADIO_RESPONSE_1_2          QTI_RADIO_IFACE_1_2(QTI_RADIO_RESPONSE_IFACE)

#define QTI_RADIO_INDICATION_1_0        QTI_RADIO_IFACE_1_0(QTI_RADIO_INDICATION_IFACE)
#define QTI_RADIO_INDICATION_1_1        QTI_RADIO_IFACE_1_1(QTI_RADIO_INDICATION_IFACE)
#define QTI_RADIO_INDICATION_1_2        QTI_RADIO_IFACE_1_2(QTI_RADIO_INDICATION_IFACE)

/*
enum RegState : int32_t {
    REGISTERED,
    NOT_REGISTERED,
    REGISTERING,
    INVALID,
};
*/

typedef enum qti_radio_reg_state {
    QTI_RADIO_REG_STATE_REGISTERED = 0,
    QTI_RADIO_REG_STATE_NOT_REGISTERED = 1,
    QTI_RADIO_REG_STATE_REGISTERING = 2,
    QTI_RADIO_REG_STATE_INVALID = 3,
} QTI_RADIO_REG_STATE;

/*
enum StatusType : int32_t {
    STATUS_DISABLED,
    STATUS_PARTIALLY_ENABLED,
    STATUS_ENABLED,
    STATUS_NOT_SUPPORTED,
    STATUS_INVALID,
};
*/

typedef enum qti_radio_status {
    QTI_RADIO_STATUS_DISABLED = 0,
    QTI_RADIO_STATUS_PARTIALLY_ENABLED = 1,
    QTI_RADIO_STATUS_ENABLED = 2,
    QTI_RADIO_STATUS_NOT_SUPPORTED = 3,
    QTI_RADIO_STATUS_INVALID = 4,
} QTI_RADIO_STATUS;

/*
enum ServiceClassStatus : int32_t {
    DISABLED,
    ENABLED,
    INVALID,
};
*/

typedef enum qti_radio_service_status {
    QTI_RADIO_SERVICE_STATUS_DISABLED = 0,
    QTI_RADIO_SERVICE_STATUS_ENABLED = 1,
    QTI_RADIO_SERVICE_STATUS_INVALID = 2,
} QTI_RADIO_SERVICE_STATUS;

/*
enum CallState : int32_t {
    CALL_ACTIVE,
    CALL_HOLDING,
    CALL_DIALING,
    CALL_ALERTING,
    CALL_INCOMING,
    CALL_WAITING,
    CALL_END,
    CALL_STATE_INVALID,
};
*/

typedef enum qti_radio_call_state {
    QTI_RADIO_CALL_STATE_ACTIVE = 0,
    QTI_RADIO_CALL_STATE_HOLDING = 1,
    QTI_RADIO_CALL_STATE_DIALING = 2,
    QTI_RADIO_CALL_STATE_ALERTING = 3,
    QTI_RADIO_CALL_STATE_INCOMING = 4,
    QTI_RADIO_CALL_STATE_WAITING = 5,
    QTI_RADIO_CALL_STATE_END = 6,
    QTI_RADIO_CALL_STATE_INVALID = 7,
} QTI_RADIO_CALL_STATE;

/*
enum IpPresentation : int32_t {
    IP_PRESENTATION_NUM_ALLOWED,
    IP_PRESENTATION_NUM_RESTRICTED,
    IP_PRESENTATION_NUM_DEFAULT,
    IP_PRESENTATION_INVALID,
};

*/

typedef enum qti_radio_ip_presentation {
    QTI_RADIO_IP_PRESENTATION_NUM_ALLOWED = 0,
    QTI_RADIO_IP_PRESENTATION_NUM_RESTRICTED = 1,
    QTI_RADIO_IP_PRESENTATION_NUM_DEFAULT = 2,
    QTI_RADIO_IP_PRESENTATION_INVALID = 3,
} QTI_RADIO_IP_PRESENTATION;

/*
enum CallType : int32_t {
    CALL_TYPE_VOICE,
    CALL_TYPE_VT_TX,
    CALL_TYPE_VT_RX,
    CALL_TYPE_VT,
    CALL_TYPE_VT_NODIR,
    CALL_TYPE_CS_VS_TX,
    CALL_TYPE_CS_VS_RX,
    CALL_TYPE_PS_VS_TX,
    CALL_TYPE_PS_VS_RX,
    CALL_TYPE_UNKNOWN,
    CALL_TYPE_SMS,
    CALL_TYPE_UT,
    CALL_TYPE_INVALID,
};

*/

typedef enum qti_radio_call_type {
    QTI_RADIO_CALL_TYPE_VOICE = 0,
    QTI_RADIO_CALL_TYPE_VT_TX = 1,
    QTI_RADIO_CALL_TYPE_VT_RX = 2,
    QTI_RADIO_CALL_TYPE_VT = 3,
    QTI_RADIO_CALL_TYPE_VT_NODIR = 4,
    QTI_RADIO_CALL_TYPE_CS_VS_TX = 5,
    QTI_RADIO_CALL_TYPE_CS_VS_RX = 6,
    QTI_RADIO_CALL_TYPE_PS_VS_TX = 7,
    QTI_RADIO_CALL_TYPE_PS_VS_RX = 8,
    QTI_RADIO_CALL_TYPE_UNKNOWN = 9,
    QTI_RADIO_CALL_TYPE_SMS = 10,
    QTI_RADIO_CALL_TYPE_UT = 11,
    QTI_RADIO_CALL_TYPE_INVALID = 12,
} QTI_RADIO_CALL_TYPE;

/*

enum CallDomain : int32_t {
    CALL_DOMAIN_UNKNOWN,
    CALL_DOMAIN_CS,
    CALL_DOMAIN_PS,
    CALL_DOMAIN_AUTOMATIC,
    CALL_DOMAIN_NOT_SET,
    CALL_DOMAIN_INVALID,
};
*/

typedef enum qti_radio_call_domain {
    QTI_RADIO_CALL_DOMAIN_UNKNOWN = 0,
    QTI_RADIO_CALL_DOMAIN_CS = 1,
    QTI_RADIO_CALL_DOMAIN_PS = 2,
    QTI_RADIO_CALL_DOMAIN_AUTOMATIC = 3,
    QTI_RADIO_CALL_DOMAIN_NOT_SET = 4,
    QTI_RADIO_CALL_DOMAIN_INVALID = 5,
} QTI_RADIO_CALL_DOMAIN;

/*

struct RegistrationInfo {
    RegState state;
    uint32_t errorCode;
    string errorMessage;
    RadioTechType radioTech;
    string pAssociatedUris;
};  
*/

typedef struct qti_radio_reg_info {
    QTI_RADIO_REG_STATE state RADIO_ALIGNED(4);
    guint32 error_code RADIO_ALIGNED(4);
    GBinderHidlString error_message RADIO_ALIGNED(8);
    guint32 radio_tech RADIO_ALIGNED(4);
    GBinderHidlString uri RADIO_ALIGNED(8);
} QtiRadioRegInfo;

/*
struct CallInfo {
    CallState state;
    uint32_t index;
    uint32_t toa;
    bool hasIsMpty;
    bool isMpty;
    bool hasIsMT;
    bool isMT;
    uint32_t als;
    bool hasIsVoice;
    bool isVoice;
    bool hasIsVoicePrivacy;
    bool isVoicePrivacy;
    string number;
    uint32_t numberPresentation;
    string name;
    uint32_t namePresentation;
    bool hasCallDetails;
};
*/

typedef struct qti_radio_call_info {
    QTI_RADIO_CALL_STATE state RADIO_ALIGNED(4);
    guint32 index RADIO_ALIGNED(4);
    guint32 toa RADIO_ALIGNED(4);
    gboolean has_is_mpty RADIO_ALIGNED(4);
    gboolean is_mpty RADIO_ALIGNED(4);
    gboolean has_is_mt RADIO_ALIGNED(4);
    gboolean is_mt RADIO_ALIGNED(4);
    guint32 als RADIO_ALIGNED(4);
    gboolean has_is_voice RADIO_ALIGNED(4);
    gboolean is_voice RADIO_ALIGNED(4);
    gboolean has_is_voice_privacy RADIO_ALIGNED(4);
    gboolean is_voice_privacy RADIO_ALIGNED(4);
    GBinderHidlString number RADIO_ALIGNED(8);
    guint32 number_presentation RADIO_ALIGNED(4);
    GBinderHidlString name RADIO_ALIGNED(8);
    guint32 name_presentation RADIO_ALIGNED(4);
    gboolean has_call_details RADIO_ALIGNED(4);
} QtiRadioCallInfo;

/*

struct ServiceStatusInfo {
    bool hasIsValid;
    bool isValid;
    ServiceType type;
    CallType callType;
    StatusType status;
    vec<uint8_t> userdata;
    uint32_t restrictCause;
    vec<StatusForAccessTech> accTechStatus;
    RttMode rttMode;
};
    
    */

typedef struct qti_radio_service_status_info {
    gboolean has_is_valid RADIO_ALIGNED(4);
    gboolean is_valid RADIO_ALIGNED(4);
    guint32 type RADIO_ALIGNED(4);
    guint32 call_type RADIO_ALIGNED(4);
    guint32 status RADIO_ALIGNED(4);
    GBinderHidlVec userdata RADIO_ALIGNED(8);
    guint32 restrict_cause RADIO_ALIGNED(4);
    GBinderHidlVec acc_tech_status RADIO_ALIGNED(8);
    guint32 rtt_mode RADIO_ALIGNED(4);
} QtiRadioServiceStatusInfo;

/*
struct CallDetails {
    CallType callType;
    CallDomain callDomain;
    uint32_t extrasLength;
    vec<string> extras;

    vec<ServiceStatusInfo> localAbility;
    vec<ServiceStatusInfo> peerAbility;
    uint32_t callSubstate;
    uint32_t mediaId;
    uint32_t causeCode;
    RttMode rttMode;
    string sipAlternateUri;
};
*/

typedef struct qti_radio_call_details {
    guint32 call_type RADIO_ALIGNED(4);
    guint32 call_domain RADIO_ALIGNED(4);
    guint32 extras_length RADIO_ALIGNED(4);

    GBinderHidlVec extras RADIO_ALIGNED(8);
    GBinderHidlVec local_ability RADIO_ALIGNED(8);
    GBinderHidlVec peer_ability RADIO_ALIGNED(8);

    guint32 call_substate RADIO_ALIGNED(4);
    guint32 media_id RADIO_ALIGNED(4);
    guint32 cause_code RADIO_ALIGNED(4);
    guint32 rtt_mode RADIO_ALIGNED(4);
    GBinderHidlString sip_alternate_uri RADIO_ALIGNED(8);
} RADIO_ALIGNED(8) QtiRadioCallDetails;

/*
struct DialRequest {
    string address;
    uint32_t clirMode;
    IpPresentation presentation;
    bool hasCallDetails;
    CallDetails callDetails;
    bool hasIsConferenceUri;
    bool isConferenceUri;
    bool hasIsCallPull;
    bool isCallPull;
    bool hasIsEncrypted;
    bool isEncrypted;
};

*/

typedef struct qti_radio_dial_request {
    GBinderHidlString address RADIO_ALIGNED(8);
    guint32 clir_mode RADIO_ALIGNED(4);
    guint32 presentation RADIO_ALIGNED(4);
    guint8 has_call_details RADIO_ALIGNED(1);
    QtiRadioCallDetails call_details RADIO_ALIGNED(8);
    guint8 has_is_conference_uri RADIO_ALIGNED(1);
    guint8 is_conference_uri RADIO_ALIGNED(1);
    guint8 has_is_call_pull RADIO_ALIGNED(1);
    guint8 is_call_pull RADIO_ALIGNED(1);
    guint8 has_is_encrypted RADIO_ALIGNED(1);
    guint8 is_encrypted RADIO_ALIGNED(1);
} RADIO_ALIGNED(8) QtiRadioDialRequest;


/* c(req, resp, callName, CALL_NAME) */
#define QTI_RADIO_EXT_IMS_CALL_1_0(c) \
    c(2, 1, dail, DAIL) \
    c(4, 11, getImsRegistrationState, GET_IMS_REG_STATE) \
    c(7, 4, requestRegistrationChange, REQ_REG_CHANGE) \
    c(31, 28, setSuppServiceNotification, SET_SUPP_SVC_NOTIFICATION)

#define QTI_RADIO_EXT_IMS_CALL_1_1(c) \
    c(41, 3, hangup_1_1, HANGUP_1_1)

#define QTI_RADIO_EXT_IMS_CALL_1_2(c) \
    c(42, 3, hangup_1_2, HANGUP_1_2) \
    c(43, 37, sendImsSms, SEND_IMS_SMS) \
    c(1, 1, acknowledgeSms, ACK_SMS)

typedef enum qti_radio_req {
    /* vendor.mediatek.hardware.qtiradioex@1.0::IqtiRadioExt */
    QTI_RADIO_REQ_SET_CALLBACK = 1, /* setCallback */
#define QTI_RADIO_REQ_(req,resp,Name,NAME) QTI_RADIO_REQ_##NAME = req,
    QTI_RADIO_EXT_IMS_CALL_1_0(QTI_RADIO_REQ_)
#undef QTI_RADIO_REQ_
} QTI_RADIO_REQ;

typedef enum ims_radio_resp {
    /* vendor.mediatek.hardware.qtiradioex@3.0::IImsRadioResponse */
#define QTI_RADIO_RESP_(req,resp,Name,NAME) QTI_RADIO_RESP_##NAME = resp,
    QTI_RADIO_EXT_IMS_CALL_1_0(QTI_RADIO_RESP_)
#undef QTI_RADIO_RESP_
} IMS_RADIO_RESP;

/* e(code, name, NAME) */
#define QTI_RADIO_IND_1_0(e) \
    e(1, onCallStateChanged, CALL_STATE_INDICATION) \
    e(2, onRing, RING_INDICATION) \
    e(3, onRingbackTone, RINGBACK_TONE_INDICATION) \
    e(4, onRegistrationChanged, REG_STATE_INDICATION) \
    e(5, onHandover, HANDOVER_INDICATION) \
    e(6, onServiceStatusChanged, SVC_STATUS_INDICATION)

typedef enum ims_radio_ind {
    /* vendor.mediatek.hardware.qtiradioex@3.0::IImsRadioIndication */
#define QTI_RADIO_IND_(code, name, NAME) QTI_RADIO_IND_##NAME = code,
    QTI_RADIO_IND_1_0(QTI_RADIO_IND_)
#undef QTI_RADIO_IND_
} IMS_RADIO_IND;

static const char*
qti_radio_ext_req_name(
    guint32 req)
{
    switch (req) {
#define QTI_RADIO_REQ_(req, resp, name, NAME) \
        case QTI_RADIO_REQ_##NAME: return #name;
    QTI_RADIO_EXT_IMS_CALL_1_0(QTI_RADIO_REQ_)
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

#endif /* QTI_RADIO_EXT_TYPES_H */

/*
 * Local Variables:
 * mode: C
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
