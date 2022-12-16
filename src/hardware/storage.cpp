#include "storage.h"

#include <map>

#include <esp_log.h>
#include <esp_vfs_fat.h>

namespace hardware
{
    namespace storage
    {
        static std::map<std::string, wl_handle_t> handles;

        void mount(type storage_type, const char *mount_point)
        {
            if (storage_type != type::internal)
                return;

            const esp_vfs_fat_mount_config_t mount_config = {
                .format_if_mount_failed = true,
                .max_files = 4,
                .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
            };

            wl_handle_t wl_handle = WL_INVALID_HANDLE;

            ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount(mount_point, "storage", &mount_config, &wl_handle));

            handles[mount_point] = wl_handle;
        }

        void unmount(const char *mount_point)
        {
            if (handles.find(mount_point) == handles.end())
                return;

            auto wl_handle = handles[mount_point];

            ESP_ERROR_CHECK(esp_vfs_fat_spiflash_unmount(mount_point, wl_handle));

            handles.erase(mount_point);
        }
    }
}