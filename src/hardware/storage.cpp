#include "storage.h"

#include <esp_log.h>
#include <esp_spiffs.h>

namespace hardware
{
    namespace storage
    {
        void mount(type storage_type, const char *mount_point)
        {
            if (storage_type != type::internal)
                return;

            if (esp_spiffs_mounted("storage"))
                return;

            const esp_vfs_spiffs_conf_t mount_config = {
                .base_path = mount_point,
                .partition_label = "storage",
                .max_files = 4,
                .format_if_mount_failed = true,
            };

            ESP_ERROR_CHECK(esp_vfs_spiffs_register(&mount_config));
        }

        void unmount(type storage_type)
        {
            if (storage_type != type::internal)
                return;

            if (!esp_spiffs_mounted("storage"))
                return;

            ESP_ERROR_CHECK(esp_vfs_spiffs_unregister("storage"));
        }
    }
}