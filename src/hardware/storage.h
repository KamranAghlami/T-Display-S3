#pragma once

namespace hardware
{
    namespace storage
    {
        enum class type
        {
            internal,
        };

        void mount(type storage_type, const char *mount_point);
        void unmount(const char *mount_point);
    }
}