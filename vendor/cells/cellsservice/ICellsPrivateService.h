#ifndef __ICELLSSERVICE_H__
#define __ICELLSSERVICE_H__

#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/BinderService.h>
#include <cutils/properties.h>
#include <utils/String8.h>

namespace android
{

class String8;
class ICellsPrivateService : public IInterface
{
protected:
    enum {
        SETPROPERTY = IBinder::FIRST_CALL_TRANSACTION,
    };

public:
    DECLARE_META_INTERFACE(CellsPrivateService);

    virtual status_t setProperty(const String8& name,const String8& value) = 0;
};

class BnCellsPrivateService : public BnInterface<ICellsPrivateService>
{
    virtual status_t onTransact(uint32_t code,
                                const Parcel& data,
                                Parcel* reply,
                                uint32_t flags = 0);
};

};

#endif
