#ifndef FONTCLASS_H
#define FONTCLASS_H
#include <vulkan/vulkan_core.h>

struct FontType {
    float u0, v0;
    float u1, v1;
};

class Font {

public:
    Font();
    ~Font();

    void Initialize();
    void Shutdown();

private:
    void LoadFontData();
    void LoadFontTexture();

};

#endif //FONTCLASS_H
