#define LOG_TAG "GuiExt"

#define MTK_LOG_ENABLE 1
#include <cutils/log.h>
#include <binder/BinderService.h>
#include <CellsPrivateService.h>

using namespace android;

int main(int /*argc*/, char** /*argv*/)
{
    ALOGI("GuiExt service start...");

    CellsPrivateService::publishAndJoinThreadPool(true);

    ProcessState::self()->setThreadPoolMaxThreadCount(4);

    ALOGD("Cells service exit...");
    return 0;
}
