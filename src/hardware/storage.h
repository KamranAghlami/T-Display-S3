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
        void unmount(type storage_type);
    }
}