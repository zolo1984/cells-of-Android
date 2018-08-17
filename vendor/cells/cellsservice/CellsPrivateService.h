#pragma GCC system_header
#ifndef __GUIEXT_SERVICE_H__
#define __GUIEXT_SERVICE_H__

#include <utils/threads.h>
#include "ICellsPrivateService.h"

namespace android
{

class String8;
class CellsPrivateService :
        public BinderService<CellsPrivateService>,
        public BnCellsPrivateService
//        public Thread
{
    friend class BinderService<CellsPrivateService>;
public:

    CellsPrivateService();
    ~CellsPrivateService();

    static char const* getServiceName() { return "CellsPrivateService"; }

    virtual status_t setProperty(const String8& name,const String8& value);

private:
};
};
#endif
