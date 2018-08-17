#define LOG_TAG "CELLSSERVICE"

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Vector.h>
#include <utils/Timers.h>
#include <utils/String8.h>

#include <binder/Parcel.h>
#include <binder/IInterface.h>

#define MTK_LOG_ENABLE 1
#include <cutils/log.h>

#include "ICellsPrivateService.h"

namespace android {

class BpCellsPrivateService : public BpInterface<ICellsPrivateService>
{
public:
    BpCellsPrivateService(const sp<IBinder>& impl) : BpInterface<ICellsPrivateService>(impl)
    {
    }

    virtual status_t setProperty(const String8& name,const String8& value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(ICellsPrivateService::getInterfaceDescriptor());
        data.writeString8(name);
        data.writeString8(value);
        status_t result = remote()->transact(SETPROPERTY, data, &reply);
        if (result != NO_ERROR) {
            ALOGE("could not set property\n");
            return result;
        }
        result = reply.readInt32();
        return result;
    }
};

IMPLEMENT_META_INTERFACE(CellsPrivateService, "CellsPrivateService");

status_t BnCellsPrivateService::onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code)
    {
        case SETPROPERTY:
        {
            CHECK_INTERFACE(ICellsPrivateService, data, reply);
            String8 name = data.readString8();
            String8 value = data.readString8();

            status_t result = setProperty(name, value);
            reply->writeInt32(result);
            return NO_ERROR;
        }
        break;
    }
    return BBinder::onTransact(code, data, reply, flags);
}

};