#define LOG_TAG "CELLSSERVICE"
#include <string.h>
#define MTK_LOG_ENABLE 1
#include <cutils/log.h>
#include <cutils/properties.h>
#include "CellsPrivateService.h"

namespace android {

#define SYSTEMPRIVATE_LOGV(x, ...) ALOGV("[CellsPrivate] " x, ##__VA_ARGS__)
#define SYSTEMPRIVATE_LOGD(x, ...) ALOGD("[CellsPrivate] " x, ##__VA_ARGS__)
#define SYSTEMPRIVATE_LOGI(x, ...) ALOGI("[CellsPrivate] " x, ##__VA_ARGS__)
#define SYSTEMPRIVATE_LOGW(x, ...) ALOGW("[CellsPrivate] " x, ##__VA_ARGS__)
#define SYSTEMPRIVATE_LOGE(x, ...) ALOGE("[CellsPrivate] " x, ##__VA_ARGS__)

CellsPrivateService::CellsPrivateService()
{
}

CellsPrivateService::~CellsPrivateService()
{
}

status_t CellsPrivateService::setProperty(const String8& name,const String8& value)
{
    SYSTEMPRIVATE_LOGD("SETPROPERTY arg %s %s", name.string(), value.string());
    status_t result = property_set(name, value);
    SYSTEMPRIVATE_LOGD("SETPROPERTY result = %d", result);
    return result;
}

};
