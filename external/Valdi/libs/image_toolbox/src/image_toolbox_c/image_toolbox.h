#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;
    int height;
} image_toolbox_size;

image_toolbox_size imagetoolbox_get_size(const char* input_path, char** error);
int imagetoolbox_convert(
    const char* input_path, const char* output_path, int outputWidth, int outputHeight, char** error);
void imagetoolbox_free_error(char* error);

#ifdef __cplusplus
}
#endif
