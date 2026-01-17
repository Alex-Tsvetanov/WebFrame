#include "djinni_c_translators.hpp"

namespace djinni::c_api {

class BinaryWithVector : public ::djinni::Binary {
public:
    BinaryWithVector(std::vector<uint8_t>&& vec) : ::djinni::Binary(vec.data(), vec.size()), _vec(std::move(vec)) {}

    ~BinaryWithVector() override = default;

private:
    std::vector<uint8_t> _vec;
};

djinni_string_ref StringTranslator::fromCpp(std::string&& str) {
    return djinni_string_new(str.c_str(), str.length());
}

djinni_string_ref StringTranslator::fromCpp(const std::string& str) {
    return djinni_string_new(str.c_str(), str.length());
}

std::string StringTranslator::toCpp(djinni_string_ref str) {
    const auto* data = djinni_string_get_data(str);
    auto length = djinni_string_get_length(str);

    return std::string(data, length);
}

DateTranslator::CType DateTranslator::fromCpp(const DateTranslator::CppType& date) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(date.time_since_epoch()).count();
    return djinni_date_new(ms);
}

DateTranslator::CppType DateTranslator::toCpp(DateTranslator::CType date) {
    auto epochMs = djinni_date_get_epoch(date);
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{epochMs}};
}

std::vector<uint8_t> BinaryTranslator::toCpp(djinni_binary_ref binary) {
    const auto* data = djinni_binary_get_data(binary);
    return std::vector<uint8_t>(data, data + djinni_binary_get_length(binary));
}

djinni_binary_ref BinaryTranslator::fromCpp(std::vector<uint8_t>&& binary) {
    Object* obj = new BinaryWithVector(std::move(binary));
    return reinterpret_cast<djinni_binary_ref>(obj);
}

djinni_binary_ref BinaryTranslator::fromCpp(const std::vector<uint8_t>& binary) {
    return djinni_binary_new_with_bytes_copy(binary.data(), binary.size());
}

} // namespace djinni::c_api
