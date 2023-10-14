#include "compiler_explorer.h"

#include "compiler.h"
#include "virtual_machine.h"
#include "type_checker.h"
#include "code_gen.h"

#include <resizable_array.inl>
#include <light_string.h>
#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <bgfx/embedded_shader.h>
#include <bx/allocator.h>
#include <bx/timer.h>
#include <bx/math.h>
#include <stdio.h>
#include <defer.h>
#include <string_builder.h>

// This defines a macro called min somehow? We should avoid it at all costs and include it last
namespace SDL {
	#include <SDL_syswm.h>
}

#include "imgui_data/vs_ocornut_imgui.bin.h"
#include "imgui_data/fs_ocornut_imgui.bin.h"
#include "imgui_data/vs_imgui_image.bin.h"
#include "imgui_data/fs_imgui_image.bin.h"


namespace {
    int selectedLine = 12;
}

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

// ***********************************************************************

static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image),
    BGFX_EMBEDDED_SHADER(fs_imgui_image),

    BGFX_EMBEDDED_SHADER_END()
};

// ***********************************************************************

struct BackendData
{
    bgfx::ShaderHandle imguiVertex;
    bgfx::ShaderHandle imguiFragment;
    bgfx::ShaderHandle imguiImageVertex;
    bgfx::ShaderHandle imguiImageFragment;

    SDL_Window* m_SdlWindow;
    bgfx::VertexLayout  m_layout;
    bgfx::ProgramHandle m_program;
    bgfx::ProgramHandle m_imageProgram;
    bgfx::TextureHandle m_texture;
    bgfx::UniformHandle s_tex;
    bgfx::UniformHandle u_imageLodEnabled;
    
    bgfx::ViewId m_mainViewId;
    ResizableArray<bgfx::ViewId> m_freeViewIds;
    bgfx::ViewId m_subViewId = 100;

    bgfx::ViewId AllocateViewId() {
        if (m_freeViewIds.count != 0) {
            const bgfx::ViewId id = m_freeViewIds[m_freeViewIds.count - 1];
            m_freeViewIds.PopBack();
            return id;
        }
        return m_subViewId++;
    }

    void FreeViewId(bgfx::ViewId id) {
        m_freeViewIds.PushBack(id);
    }
};

// ***********************************************************************

struct ViewportData 
{
    bgfx::FrameBufferHandle frameBufferHandle;
    bgfx::ViewId viewId = 0;
    uint16_t width = 0;
    uint16_t height = 0;
};

// ***********************************************************************

inline bool CheckAvailTransientBuffers(uint32_t _numVertices, const bgfx::VertexLayout& _layout, uint32_t _numIndices)
{
    return _numVertices == bgfx::getAvailTransientVertexBuffer(_numVertices, _layout)
        && (0 == _numIndices || _numIndices == bgfx::getAvailTransientIndexBuffer(_numIndices) )
        ;
}

// ***********************************************************************

void* GetNativeWindowHandle(SDL_Window* pWindow) {
    SDL::SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(pWindow, &wmi)) {
        return NULL;
    }
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#if ENTRY_CONFIG_USE_WAYLAND
    wl_egl_window* win_impl = (wl_egl_window*)SDL_GetWindowData(pWindow, "wl_egl_window");
    if (!win_impl) {
        int width, height;
        SDL_GetWindowSize(pWindow, &width, &height);
        struct wl_surface* surface = wmi.info.wl.surface;
        if (!surface)
            return nullptr;
        win_impl = wl_egl_window_create(surface, width, height);
        SDL_SetWindowData(pWindow, "wl_egl_window", win_impl);
    }
    return (void*)(uintptr_t)win_impl;
#else
    return (void*)wmi.info.x11.window;
#endif
#elif BX_PLATFORM_OSX
    return wmi.info.cocoa.window;
#elif BX_PLATFORM_WINDOWS
    return wmi.info.win.window;
#endif // BX_PLATFORM_
}

// ***********************************************************************

void OnCreateWindow(ImGuiViewport* pViewport) 
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* pBackend = (BackendData*)io.BackendRendererUserData;

    ViewportData* pData = new ViewportData();
    pViewport->RendererUserData = pData;
    // Setup view id and size
    pData->viewId = pBackend->AllocateViewId();
    pData->width = bx::max<uint16_t>((uint16_t)pViewport->Size.x, 1);
    pData->height = bx::max<uint16_t>((uint16_t)pViewport->Size.y, 1);
    // Create frame buffer
    pData->frameBufferHandle = bgfx::createFrameBuffer(GetNativeWindowHandle((SDL_Window*)pViewport->PlatformHandle), uint16_t(pData->width * pViewport->DrawData->FramebufferScale.x), uint16_t(pData->height * pViewport->DrawData->FramebufferScale.y));
    // Set frame buffer
    bgfx::setViewFrameBuffer(pData->viewId, pData->frameBufferHandle);
}

// ***********************************************************************

void OnDestroyWindow(ImGuiViewport* pViewport)
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* pBackend = (BackendData*)io.BackendRendererUserData;

    if (ViewportData* pData = (ViewportData*)pViewport->RendererUserData; pData)
    {
        pViewport->RendererUserData = nullptr;
        pBackend->FreeViewId(pData->viewId);
        bgfx::destroy(pData->frameBufferHandle);
        pData->frameBufferHandle.idx = bgfx::kInvalidHandle;
        delete pData;
    }
}

// ***********************************************************************

void OnSetWindowSize(ImGuiViewport* pViewport, ImVec2 size) {
    OnDestroyWindow(pViewport);
    OnCreateWindow(pViewport);
}

// ***********************************************************************

void RenderView(const bgfx::ViewId viewId, ImDrawData* pDrawData, uint32_t clearColor)
{
    ImGuiIO& io = ImGui::GetIO();
    BackendData* pBackend = (BackendData*)io.BackendRendererUserData;

    if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0)
    {
        return;
    }

    if (clearColor) {
        bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
    }
    bgfx::touch(viewId);
    bgfx::setViewName(viewId, "ImGui");
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);
    
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(pDrawData->DisplaySize.x * pDrawData->FramebufferScale.x);
    int fb_height = (int)(pDrawData->DisplaySize.y * pDrawData->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    bgfx::setViewName(viewId, "ImGui");
    bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);

    const bgfx::Caps* caps = bgfx::getCaps();
    {
        float ortho[16];
        float x = pDrawData->DisplayPos.x;
        float y = pDrawData->DisplayPos.y;
        float width = pDrawData->DisplaySize.x;
        float height = pDrawData->DisplaySize.y;

        bx::mtxOrtho(ortho, x, x + width, y + height, y, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
        bgfx::setViewTransform(viewId, NULL, ortho);
        bgfx::setViewRect(viewId, 0, 0, uint16_t(width), uint16_t(height) );
    }

    const ImVec2 clipPos   = pDrawData->DisplayPos;       // (0,0) unless using multi-viewports
    const ImVec2 clipScale = pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int32_t ii = 0, num = pDrawData->CmdListsCount; ii < num; ++ii)
    {
        bgfx::TransientVertexBuffer tvb;
        bgfx::TransientIndexBuffer tib;

        const ImDrawList* drawList = pDrawData->CmdLists[ii];
        uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
        uint32_t numIndices  = (uint32_t)drawList->IdxBuffer.size();

        if (!CheckAvailTransientBuffers(numVertices, pBackend->m_layout, numIndices) )
        {
            // not enough space in transient buffer just quit drawing the rest...
            break;
        }

        bgfx::allocTransientVertexBuffer(&tvb, numVertices, pBackend->m_layout);
        bgfx::allocTransientIndexBuffer(&tib, numIndices, sizeof(ImDrawIdx) == 4);

        ImDrawVert* verts = (ImDrawVert*)tvb.data;
        bx::memCopy(verts, drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert) );

        ImDrawIdx* indices = (ImDrawIdx*)tib.data;
        bx::memCopy(indices, drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx) );

        bgfx::Encoder* encoder = bgfx::begin();

        for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
        {
            if (cmd->UserCallback)
            {
                cmd->UserCallback(drawList, cmd);
            }
            else if (0 != cmd->ElemCount)
            {
                uint64_t state = 0
                    | BGFX_STATE_WRITE_RGB
                    | BGFX_STATE_WRITE_A
                    | BGFX_STATE_MSAA
                    | BGFX_STATE_BLEND_ALPHA
                    ;

                bgfx::TextureHandle th = pBackend->m_texture;
                bgfx::ProgramHandle program = pBackend->m_program;

                if (NULL != cmd->TextureId)
                {
                    union { ImTextureID ptr; struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; } texture = { cmd->TextureId };
                    state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
                        ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
                        : BGFX_STATE_NONE
                        ;
                    th = texture.s.handle;
                    if (0 != texture.s.mip)
                    {
                        const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
                        bgfx::setUniform(pBackend->u_imageLodEnabled, lodEnabled);
                        program = pBackend->m_imageProgram;
                    }
                }
                else
                {
                    state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
                }

                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clipRect;
                clipRect.x = (cmd->ClipRect.x - clipPos.x) * clipScale.x;
                clipRect.y = (cmd->ClipRect.y - clipPos.y) * clipScale.y;
                clipRect.z = (cmd->ClipRect.z - clipPos.x) * clipScale.x;
                clipRect.w = (cmd->ClipRect.w - clipPos.y) * clipScale.y;

                if (clipRect.x <  fb_width
                &&  clipRect.y <  fb_height
                &&  clipRect.z >= 0.0f
                &&  clipRect.w >= 0.0f)
                {
                    const uint16_t xx = uint16_t(bx::max(clipRect.x, 0.0f) );
                    const uint16_t yy = uint16_t(bx::max(clipRect.y, 0.0f) );
                    encoder->setScissor(xx, yy
                            , uint16_t(bx::min(clipRect.z, 65535.0f)-xx)
                            , uint16_t(bx::min(clipRect.w, 65535.0f)-yy)
                            );

                    encoder->setState(state);
                    encoder->setTexture(0, pBackend->s_tex, th);
                    encoder->setVertexBuffer(0, &tvb, 0, numVertices);
                    encoder->setIndexBuffer(&tib, cmd->IdxOffset, cmd->ElemCount);
                    encoder->submit(viewId, program);
                }
            }
        }

        bgfx::end(encoder);
    }
}

// ***********************************************************************

void OnRenderWindow(ImGuiViewport* pViewport, void*)
{
    if(ViewportData* pData = (ViewportData*)pViewport->RendererUserData; pData)
    {
        RenderView(pData->viewId, pViewport->DrawData, !(pViewport->Flags & ImGuiViewportFlags_NoRendererClear) ? 0x000000ff : 0);
    }
}

// ***********************************************************************

bool ProcessEvent(SDL_Event& event)
{
    if (ImGui_ImplSDL2_ProcessEvent(&event))
        return true;
    return false;
}

// ***********************************************************************

void DrawAstExpression(Ast::Expression* pExpr);
void DrawAstStatements(ResizableArray<Ast::Statement*>& statements);

void DrawAstStatement(Ast::Statement* pStmt) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedLine+1 == pStmt->line)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;      
    
    switch (pStmt->nodeKind) {
        case Ast::NodeKind::Declaration: {
            Ast::Declaration* pDecl = (Ast::Declaration*)pStmt;

            if (ImGui::TreeNodeEx(pDecl, nodeFlags, "%sDeclaration - %s", pDecl->isConstantDeclaration ? "Const" : "",  pDecl->identifier.pData)) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }


                if (pDecl->pTypeAnnotation) {
                    DrawAstExpression(pDecl->pTypeAnnotation);
                }
                else if (pDecl->pInitializerExpr)
                    ImGui::Text("Type: inferred as %s", pDecl->pInitializerExpr->pType->name.pData);

                if (pDecl->pInitializerExpr) {
                    DrawAstExpression(pDecl->pInitializerExpr);
                }
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::Print: {
            Ast::Print* pPrint = (Ast::Print*)pStmt;
            if (ImGui::TreeNodeEx(pPrint, nodeFlags, "Print Statement")) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }
                DrawAstExpression(pPrint->pExpr);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::Return: {
            Ast::Return* pReturn = (Ast::Return*)pStmt;
            if (ImGui::TreeNodeEx(pReturn, nodeFlags, "Return Statement")) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }
                if (pReturn->pExpr) {
                    DrawAstExpression(pReturn->pExpr);
                }
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::ExpressionStmt: {
            Ast::ExpressionStmt* pExprStmt = (Ast::ExpressionStmt*)pStmt;
            if (ImGui::TreeNodeEx(pExprStmt, nodeFlags, "Expression Statement")) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }
                DrawAstExpression(pExprStmt->pExpr);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::If: {
            Ast::If* pIf = (Ast::If*)pStmt;
            if (ImGui::TreeNodeEx(pIf, nodeFlags, "If Statement")) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }
                DrawAstExpression(pIf->pCondition);
                DrawAstStatement(pIf->pThenStmt);
                if (pIf->pElseStmt)
                    DrawAstStatement(pIf->pElseStmt);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::While: {
            Ast::While* pWhile = (Ast::While*)pStmt;
            if (ImGui::TreeNodeEx(pWhile, nodeFlags, "While Statement")) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }
                DrawAstExpression(pWhile->pCondition);
                DrawAstStatement(pWhile->pBody);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::Block: {
            Ast::Block* pBlock = (Ast::Block*)pStmt;
            if (ImGui::TreeNodeEx(pBlock, nodeFlags, "Block Statement")) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }
                DrawAstStatements(pBlock->declarations);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::BadStatement: {
            Ast::BadStatement* pBad = (Ast::BadStatement*)pStmt;
            if (ImGui::TreeNodeEx(pBad, nodeFlags, "Bad Statement")) {
                if (ImGui::IsItemClicked()) { selectedLine = pStmt->line-1; }
                ImGui::TreePop();
            }
            break;
        }
        default:
            break;
    }
}

// ***********************************************************************

void DrawExprProperties(Ast::Expression* pExpr) {
    String nodeTypeStr = "unresolved";
    if (pExpr->pType) {
        nodeTypeStr = pExpr->pType->name;
    }
    if (pExpr->isConstant) {
        if (CheckTypesIdentical(pExpr->pType, GetF32Type()))
            ImGui::Text("Type: %s Constant: %s Value: %f", nodeTypeStr.pData, pExpr->isConstant ? "true" : "false", pExpr->constantValue.f32Value);
        else if (CheckTypesIdentical(pExpr->pType, GetI32Type()))
            ImGui::Text("Type: %s Constant: %s Value: %i", nodeTypeStr.pData, pExpr->isConstant ? "true" : "false", pExpr->constantValue.i32Value);
        else if (CheckTypesIdentical(pExpr->pType, GetBoolType()))
            ImGui::Text("Type: %s Constant: %s Value: %s", nodeTypeStr.pData, pExpr->isConstant ? "true" : "false", pExpr->constantValue.boolValue ? "true" : "false");
        else if (CheckTypesIdentical(pExpr->pType, GetTypeType()) && pExpr->constantValue.pTypeInfo)
            ImGui::Text("Type: %s Constant: %s Value: %s", nodeTypeStr.pData, pExpr->isConstant ? "true" : "false", pExpr->constantValue.pTypeInfo->name.pData);
        else
            ImGui::Text("Type: %s Constant: %s Value: unable to print", nodeTypeStr.pData, pExpr->isConstant ? "true" : "false");
    } else {
        ImGui::Text("Type: %s Constant: %s", nodeTypeStr.pData, pExpr->isConstant ? "true" : "false");
    }
}

// ***********************************************************************

void DrawAstExpression(Ast::Expression* pExpr) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selectedLine+1 == pExpr->line)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;      
    
    // What we really want is
    // if a node is selected, then the cursor is already at the right position
    // So you'll have to do a split draw lists thing again
    // Then draw the tree node fully, record the cursor position again
    // Then draw a rect start to end, covering the whole area

    switch (pExpr->nodeKind) {
        case Ast::NodeKind::Identifier: {
            Ast::Identifier* pIdentifier = (Ast::Identifier*)pExpr;
            if (ImGui::TreeNodeEx(pIdentifier, nodeFlags, "Identifier - %s", pIdentifier->identifier.pData)) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::FunctionType: {
            Ast::FunctionType* pFuncType = (Ast::FunctionType*)pExpr;
            if (ImGui::TreeNodeEx(pFuncType, nodeFlags, "Function Type")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                for (Ast::Node* pParam : pFuncType->params) {
                    if (pParam->nodeKind == Ast::NodeKind::Identifier) {
                        DrawAstExpression((Ast::Expression*)pParam);
                    } else if (pParam->nodeKind == Ast::NodeKind::Declaration) {
                        DrawAstStatement((Ast::Declaration*)pParam);
                    }
                }
                if (pFuncType->pReturnType)
                    DrawAstExpression(pFuncType->pReturnType);
                ImGui::TreePop();
            }
            break;
        } 
        case Ast::NodeKind::Type: {
            Ast::Type* pType = (Ast::Type*)pExpr;
            if (ImGui::TreeNodeEx(pType, nodeFlags, "Type Literal")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::VariableAssignment: {
            Ast::VariableAssignment* pAssignment = (Ast::VariableAssignment*)pExpr;
            if (ImGui::TreeNodeEx(pAssignment, nodeFlags, "Variable Assignment")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstExpression(pAssignment->pIdentifier);
                DrawAstExpression(pAssignment->pAssignment);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::Literal: {
            Ast::Literal* pLiteral = (Ast::Literal*)pExpr;
            if (ImGui::TreeNodeEx(pLiteral, nodeFlags, "Literal")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::Function: {
            Ast::Function* pFunction = (Ast::Function*)pExpr;
            if (ImGui::TreeNodeEx(pFunction, nodeFlags, "Function")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstExpression(pFunction->pFuncType);
                DrawAstStatement(pFunction->pBody);
                ImGui::TreePop();
            }
            break;
        }
		case Ast::NodeKind::Structure: {
			Ast::Structure* pStruct = (Ast::Structure*)pExpr;
            if (ImGui::TreeNodeEx(pStruct, nodeFlags, "Struct")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstStatements(pStruct->members);
                ImGui::TreePop();
            }
			break;
		}
        case Ast::NodeKind::Grouping: {
            Ast::Grouping* pGroup = (Ast::Grouping*)pExpr;
            if (ImGui::TreeNodeEx(pGroup, nodeFlags, "Grouping")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstExpression(pGroup->pExpression);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::Binary: {
            Ast::Binary* pBinary = (Ast::Binary*)pExpr;
            if (ImGui::TreeNodeEx(pBinary, nodeFlags, "Binary")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                switch (pBinary->op) {
                    case Operator::Add:
                        ImGui::Text("Operator: +");
                        break;
                    case Operator::Subtract:
                        ImGui::Text("Operator: -");
                        break;
                    case Operator::Divide:
                        ImGui::Text("Operator: /");
                        break;
                    case Operator::Multiply:
                        ImGui::Text("Operator: *");
                        break;
                    case Operator::Greater:
                        ImGui::Text("Operator: >");
                        break;
                    case Operator::Less:
                        ImGui::Text("Operator: <");
                        break;
                    case Operator::GreaterEqual:
                        ImGui::Text("Operator: >=");
                        break;
                    case Operator::LessEqual:
                        ImGui::Text("Operator: <=");
                        break;
                    case Operator::Equal:
                        ImGui::Text("Operator: ==");
                        break;
                    case Operator::NotEqual:
                        ImGui::Text("Operator: !=");
                        break;
                    case Operator::And:
                        ImGui::Text("Operator: &&");
                        break;
                    case Operator::Or:
                        ImGui::Text("Operator: ||");
                        break;
                    default:
                        break;
                }
                DrawAstExpression(pBinary->pLeft);
                DrawAstExpression(pBinary->pRight);
                ImGui::TreePop();
            }
            break;
        }
        case Ast::NodeKind::Unary: {
            Ast::Unary* pUnary = (Ast::Unary*)pExpr;
            if (ImGui::TreeNodeEx(pUnary, nodeFlags, "Unary")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                switch (pUnary->op) {
                    case Operator::UnaryMinus:
                        ImGui::Text("Operator: -");
                        break;
                    case Operator::Not:
                        ImGui::Text("Operator: !");
                        break;
                    default:
                        break;
                }
                DrawAstExpression(pUnary->pRight);
                ImGui::TreePop();
            }
            break;
        }
		case Ast::NodeKind::Cast: {
			Ast::Cast* pCast = (Ast::Cast*)pExpr;
            if (ImGui::TreeNodeEx(pCast, nodeFlags, "Cast")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstExpression(pCast->pTypeExpr);
                DrawAstExpression(pCast->pExprToCast);
                ImGui::TreePop();
            }
			break;
		}
        case Ast::NodeKind::Call: {
            Ast::Call* pCall = (Ast::Call*)pExpr;
            if (ImGui::TreeNodeEx(pCall, nodeFlags, "Call")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstExpression(pCall->pCallee);
                for (Ast::Expression* pArg : pCall->args) {
                    DrawAstExpression(pArg);
                }
                ImGui::TreePop();
            }
            break;
        }
		case Ast::NodeKind::GetField: {
			Ast::GetField* pGetField = (Ast::GetField*)pExpr;
            if (ImGui::TreeNodeEx(pGetField, nodeFlags, "Get Field - %s", pGetField->fieldName.pData)) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstExpression(pGetField->pTarget);
                ImGui::TreePop();
            }
			break;
		}
		case Ast::NodeKind::SetField: {
			Ast::SetField* pSetField = (Ast::SetField*)pExpr;
            if (ImGui::TreeNodeEx(pSetField, nodeFlags, "Set Field - %s", pSetField->fieldName.pData)) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                DrawAstExpression(pSetField->pTarget);
                DrawAstExpression(pSetField->pAssignment);
                ImGui::TreePop();
            }
			break;
		}
        case Ast::NodeKind::BadExpression: {
			Ast::BadExpression* pBad = (Ast::BadExpression*)pExpr;
            if (ImGui::TreeNodeEx(pBad, nodeFlags, "Bad Expression")) {
                if (ImGui::IsItemClicked()) { selectedLine = pExpr->line-1; }
                DrawExprProperties(pExpr);
                ImGui::TreePop();
            }
			break;
		}
        default:
            break;
    }
}

// ***********************************************************************

void DrawAstStatements(ResizableArray<Ast::Statement*>& statements) {
    for (size_t i = 0; i < statements.count; i++) {
        Ast::Statement* pStmt = statements[i];
        DrawAstStatement(pStmt);
    }
}

// ***********************************************************************

void DrawByteCodeForFunction(Program* pProgram, Function* pFunc) {
    ImDrawList* pDrawList = ImGui::GetWindowDrawList();
    uint8_t* pInstructionPointer = pFunc->code.pData;

    ImGui::Text("\n");
    ImGui::Text("---- Function %s", pFunc->name.pData);

    uint32_t currentLine = -1;
    uint32_t lineCounter = 0;

    while (pInstructionPointer < pFunc->code.end()) {
        if (currentLine != pFunc->dbgLineInfo[lineCounter]) {
            currentLine = pFunc->dbgLineInfo[lineCounter];
        }

        uint8_t offset;
        String output = DisassembleInstruction(pProgram, pInstructionPointer, offset);

        ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

        ImVec2 lineStartPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y);
        ImVec2 size = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, output.pData, output.pData + output.length);
        ImVec2 lineEndPos = ImVec2(lineStartPos.x + ImGui::GetContentRegionAvail().x, lineStartPos.y + size.y);

        if (selectedLine+1 == currentLine) {
            pDrawList->AddRectFilled(lineStartPos, lineEndPos, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrabActive)));
        }

        if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(lineStartPos, lineEndPos)) {
            pDrawList->AddRectFilled(lineStartPos, lineEndPos, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab)));

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                selectedLine = currentLine-1;
            }
        }

        pDrawList->AddText(lineStartPos, 0xffffffff, output.pData, output.pData + output.length);
        cursorScreenPos.y += size.y;
        
        ImGui::SetCursorScreenPos(cursorScreenPos);
        FreeString(output);

        pInstructionPointer += offset;
        lineCounter += offset;
    }
}

// ***********************************************************************

void DrawScopes(Scope* pScope) {
    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selectedLine+1 == pScope->startLine)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    if (ImGui::TreeNodeEx(pScope, nodeFlags, "Scope - Kind: %s", ScopeKind::ToString(pScope->kind))) {
        if (ImGui::IsItemClicked()) { selectedLine = pScope->startLine-1; }

        // TODO: Should really make a hashmap iterator
        for (size_t i = 0; i < pScope->entities.tableSize; i++) { 
            HashNode<String, Entity*>& node = pScope->entities.pTable[i];
            if (node.hash != UNUSED_HASH) {
                ImGuiTreeNodeFlags entityNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                Entity* pEntity = node.value;

                if (selectedLine+1 == pEntity->pDeclaration->line)
                    entityNodeFlags |= ImGuiTreeNodeFlags_Selected;      

                String entityTypeStr = "unresolved";
                if (pEntity->pType && pEntity->status == EntityStatus::Resolved) {
                    entityTypeStr = pEntity->pType->name;
                }

                ImGui::TreeNodeEx(pEntity, entityNodeFlags, "Entity - Name: %s Type: %s Kind: %s", pEntity->name.pData, entityTypeStr.pData, EntityKind::ToString(pEntity->kind));

                if (ImGui::IsItemClicked())
                    selectedLine = pEntity->pDeclaration->line-1; 
            }
        }

        for (Scope* pChildScope : pScope->children) {
            DrawScopes(pChildScope);
        }
        ImGui::TreePop();
    }
}

// ***********************************************************************

void UpdateCompilerExplorer(Compiler& compiler, ResizableArray<String>& lines) {
    // Draw UI
   	ImGuiViewport* pViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(pViewport->WorkPos);
	ImGui::SetNextWindowSize(pViewport->WorkSize);
	ImGui::SetNextWindowViewport(pViewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;;

	bool open = true;
	ImGui::Begin("Root", &open, window_flags);

	ImGui::PopStyleVar(3);
	
	ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    ImGui::SetNextWindowDockID(ImGui::GetID("MainDockspace"), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Source Code")) {
        ImGui::BeginChild("Source Code Editor", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImDrawList* pDrawList = ImGui::GetWindowDrawList();

        char* pCodeCurrent = compiler.code.pData;
        char* pCodeEnd = pCodeCurrent + compiler.code.length; 

        int nLines = lines.count;

        StringBuilder builder;
        char* pBuilderBasePtr = builder.pData;
        builder.AppendFormat(" %d ", nLines);
        String lineNumberString = builder.CreateString(false);
        defer(FreeString(lineNumberString));
        ImVec2 lineNumberMaxSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, lineNumberString.pData, lineNumberString.pData + lineNumberString.length);
        float widestColumn = 0;

        for (uint32_t i = 0; i < lines.count; i++) {
            String line = lines[i];
            ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();

            ImVec2 lineStartPos = ImVec2(cursorScreenPos.x + lineNumberMaxSize.x, cursorScreenPos.y);
            ImVec2 size = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, line.pData, line.pData + line.length);
            ImVec2 lineEndPos = ImVec2(lineStartPos.x + ImGui::GetContentRegionAvail().x, lineStartPos.y + size.y);

            if (selectedLine == i) {
                pDrawList->AddRectFilled(lineStartPos, lineEndPos, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrabActive)));
            }

            if (ImGui::IsWindowFocused() && ImGui::IsMouseHoveringRect(lineStartPos, lineEndPos)) {
                pDrawList->AddRectFilled(lineStartPos, lineEndPos, ImColor(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab)));

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    selectedLine = i;
                }
            }

            // Draw line number
            builder.length = 0;
            builder.AppendFormat("%*d  ", (int)floor(log10(nLines) + 1), i + 1);
            lineNumberString = builder.ToExistingString(false, lineNumberString);
            pDrawList->AddText(cursorScreenPos, 0xffffffff, lineNumberString.pData, lineNumberString.pData + lineNumberString.length);

            // Draw a line
            pDrawList->AddText(lineStartPos, 0xffffffff, line.pData, line.pData + line.length);
            cursorScreenPos.y += size.y;
            
            if (size.x > widestColumn) {
                widestColumn = size.x;
            }
            ImGui::SetCursorScreenPos(cursorScreenPos);
        }
        ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x + widestColumn + lineNumberMaxSize.x, ImGui::GetCursorScreenPos().y));

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::SetNextWindowDockID(ImGui::GetID("MainDockspace"), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("AST")) {
        // Draw AST
        DrawAstStatements(compiler.syntaxTree);
    }
    ImGui::End();

    ImGui::SetNextWindowDockID(ImGui::GetID("MainDockspace"), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Code Gen")) {
        if (compiler.errorState.errors.count == 0) {         
            DrawByteCodeForFunction(compiler.pProgram, compiler.pProgram->pMainModuleFunction);

            for (uint32_t i = 0; i < compiler.pProgram->constantTable.count; i++) {
                TypeInfo* pType = compiler.pProgram->dbgConstantsTypes[i];
                if (pType->tag == TypeInfo::TypeTag::Function) {
                    if (compiler.pProgram->constantTable[i].pFunction)
                        DrawByteCodeForFunction(compiler.pProgram, compiler.pProgram->constantTable[i].pFunction);
                }
            }
        }
    }
    ImGui::End();

    ImGui::SetNextWindowDockID(ImGui::GetID("MainDockspace"), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Scopes")) {
        if (compiler.pGlobalScope) {
            DrawScopes(compiler.pGlobalScope);
        }
    }
    ImGui::End();

    ImGui::End();
}

// ***********************************************************************

void RunCompilerExplorer() {
	bool appRunning{ true };
	float deltaTime;
	uint64_t frameStartTime;
	const bgfx::ViewId kClearView{ 255 };
	int width {2000};
	int height {1200};

	// Init graphics, window, imgui etc
	SDL_Window* pWindow = SDL_CreateWindow(
		"Compiler Explorer",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WINDOW_RESIZABLE
	);

	SDL::SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL::SDL_GetWindowWMInfo(pWindow, &wmInfo);
	SDL::HWND hwnd = wmInfo.info.win.window;

	bgfx::Init init;
	init.type = bgfx::RendererType::Direct3D11;
	init.platformData.ndt = NULL;
	init.platformData.nwh = wmInfo.info.win.window;

	bgfx::renderFrame();

	bgfx::init(init);
	bgfx::setViewClear(kClearView, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x404040ff, 1.0f, 0);
	bgfx::setViewRect(kClearView, 0, 0, width, height);
	bgfx::reset(width, height, BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X8);
	appRunning = true;
	deltaTime = 0.016f;

	IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    ImGui_ImplSDL2_InitForD3D(pWindow);

    IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    BackendData* pBackend = IM_NEW(BackendData)();
    io.BackendRendererUserData = (void*)pBackend;
    io.BackendRendererName = "imgui_bgfx";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    // init vars, create shader programs, textures load fonts
    pBackend->m_SdlWindow = pWindow;
    pBackend->m_mainViewId = 255;

    bgfx::RendererType::Enum type = bgfx::getRendererType();
    pBackend->imguiVertex = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui");
    pBackend->imguiFragment = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui");
    pBackend->imguiImageVertex = bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image");
    pBackend->imguiImageFragment = bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image");

    pBackend->m_program = bgfx::createProgram(pBackend->imguiVertex, pBackend->imguiFragment, false);

    pBackend->u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);
    pBackend->m_imageProgram = bgfx::createProgram(pBackend->imguiImageVertex, pBackend->imguiImageFragment, false);

    pBackend->m_layout
        .begin()
        .add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
        .end();

    pBackend->s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    config.MergeMode = false;

    const ImWchar* ranges = io.Fonts->GetGlyphRangesCyrillic();
    ImFont* pFont = io.Fonts->AddFontFromFileTTF("imgui_data/Consolas.ttf", 15.0f, &config, ranges);
    ImGui::StyleColorsDark();

    uint8_t* data;
    int32_t imgWidth;
    int32_t imgHeight;
    io.Fonts->GetTexDataAsRGBA32(&data, &imgWidth, &imgHeight);

    pBackend->m_texture = bgfx::createTexture2D(
            (uint16_t)imgWidth
        , (uint16_t)imgHeight
        , false
        , 1
        , bgfx::TextureFormat::BGRA8
        , 0
        , bgfx::copy(data, imgWidth*imgHeight*4)
        );

    io.Fonts->TexID = (void*)(intptr_t)pBackend->m_texture.idx;

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = OnCreateWindow;
    platform_io.Renderer_DestroyWindow = OnDestroyWindow;
    platform_io.Renderer_SetWindowSize = OnSetWindowSize;
    platform_io.Renderer_RenderWindow = OnRenderWindow;

    // Load a code file and compile it for analysis
    FILE* pFile;
    fopen_s(&pFile, "test.ps", "rb");
    if (pFile == NULL) {
        return;
    }

	Compiler compiler;
    {
        uint32_t size;
        fseek(pFile, 0, SEEK_END);
        size = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);

        compiler.code = AllocString(size, &compiler.compilerMemory);
        fread(compiler.code.pData, size, 1, pFile);
        fclose(pFile);
    }
	compiler.bPrintAst = false;
	compiler.bPrintByteCode = false;
	CompileCode(compiler);

    // if (compiler.errorState.errors.count > 0) {
	// 	compiler.errorState.ReportCompilationResult();
	// }

    ResizableArray<String> lines;
    defer(lines.Free());

    // Write some code to split compiler.code into lines, putting it into the lines array
    char* pCodeCurrent = compiler.code.pData;
    char* pCodeEnd = pCodeCurrent + compiler.code.length;
    while (pCodeCurrent < pCodeEnd && *pCodeCurrent != '\0') {
        char* pLineStart = pCodeCurrent;
        while(*pCodeCurrent != '\n' && *pCodeCurrent != '\0') {
            pCodeCurrent++;
        }
        String line = compiler.code.SubStr(pLineStart - compiler.code.pData, pCodeCurrent - pLineStart);
        lines.PushBack(line);
        pCodeCurrent++;
    }

	while (appRunning) {
		frameStartTime = SDL_GetPerformanceCounter();
		// Deal with events
		SDL_Event event;
		while (SDL_PollEvent(&event)){
			ProcessEvent(event);
			
			switch (event.type) {
				case SDL_WINDOWEVENT:
					switch (event.window.event) {
						case SDL_WINDOWEVENT_RESIZED:
							// Update the bgfx framebuffer to match the new window size
							bgfx::reset(event.window.data1, event.window.data2, BGFX_RESET_VSYNC);
							break;
						default:
							break;
					}
					break;
				case SDL_QUIT:
					appRunning = false;
					break;
				default:
					break;
			}
		}
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(pFont);

		//ImGui::ShowDemoWindow();
        UpdateCompilerExplorer(compiler, lines);

        ImGui::PopFont();
        ImGui::Render();
        RenderView(pBackend->m_mainViewId, ImGui::GetDrawData(), 0);
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

		bgfx::touch(kClearView);
		bgfx::frame();

		float targetFrameTime = 0.0166f;
		float realframeTime = float(SDL_GetPerformanceCounter() - frameStartTime) / SDL_GetPerformanceFrequency();
		if (realframeTime < targetFrameTime)
		{
			unsigned int waitTime = int((targetFrameTime - realframeTime) * 1000.0);
			SDL_Delay(waitTime);
		}

		deltaTime = float(SDL_GetPerformanceCounter() - frameStartTime) / SDL_GetPerformanceFrequency();
	}

    bgfx::destroy(pBackend->s_tex);
    bgfx::destroy(pBackend->m_texture);

    bgfx::destroy(pBackend->u_imageLodEnabled);
    bgfx::destroy(pBackend->m_imageProgram);
    bgfx::destroy(pBackend->m_program);

    ImGui_ImplSDL2_Shutdown();

	bgfx::shutdown();
}