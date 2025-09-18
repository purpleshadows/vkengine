#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
namespace Core::Shaders {

    enum class Stage : uint8_t { Vertex, Fragment, Compute, Geometry, TessCtrl, TessEval };

    inline EShLanguage toESh(Stage st) {
        switch (st) {
            case Stage::Vertex:   return EShLangVertex;
            case Stage::Fragment: return EShLangFragment;
            case Stage::Compute:  return EShLangCompute;
            case Stage::Geometry: return EShLangGeometry;
            case Stage::TessCtrl: return EShLangTessControl;
            case Stage::TessEval: return EShLangTessEvaluation;
        }
        return EShLangVertex;
    }
}