#define LOG_TAG "GuiExt"

#define MTK_LOG_ENABLE 1
#include <cutils/log.h>
#include <binder/BinderService.h>
#include "ICellsPrivateService.h"

using namespace android;

int main(int argc, char** argv)
{
    const sp<IServiceManager> sm = otherdefaultServiceManager();
    if (sm != NULL) {
        sp<IBinder> binder = sm->checkService(String16("CellsPrivateService"));
        if (binder != NULL) {
            sp<ICellsPrivateService> pCellsPrivateService = interface_cast<ICellsPrivateService>(binder);
            if(pCellsPrivateService == NULL){
                ALOGE("could not get service CellsPrivateService \n");
                return 0;
            }

            if (argc == 2) {
                char value[PROPERTY_VALUE_MAX];
                property_get(argv[1], value, "");
                pCellsPrivateService->setProperty(android::String8(argv[1]),android::String8(value));
            }
        }
    }

    return 0;
}
