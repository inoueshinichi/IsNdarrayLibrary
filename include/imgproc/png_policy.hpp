#ifndef IS_IMGPROC_PNG_POLICY_HPP
#define IS_IMGPROC_PNG_POLICY_HPP
#include "imgproc/common.hpp"

#include <cstdint>

namespace Is
{
    namespace imgproc
    {
        namespace format_policy
        {
            using std::string;
            using byte = unsigned char;

            class IMGPROC_API PngFilePolicy
            {
            public:
                PngFilePolicy();
                ~PngFilePolicy();
                PngFilePolicy(const PngFilePolicy&) = delete;
                PngFilePolicy& operator=(const PngFilePolicy&) = delete;
                PngFilePolicy(PngFilePolicy&&) = default;
                PngFilePolicy& operator=(PngFilePolicy&&) = default;

                void set_data(byte* data, int insert_color = -1);
                void get_data(byte* data, int extract_color = -1);
                void save(const string& filename, byte* data, int32_t width, int32_t height, int32_t channels, bool is_dump);
                void load(const string& filename, int32_t& width, int32_t& height, int32_t& channels, bool is_dump);
                void dump() const;
            };
        } // namespace format_policy
    } // namespace imgproc
}
#endif