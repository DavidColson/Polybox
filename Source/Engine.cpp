// Copyright 2020-2021 David Colson. All rights reserved.

#include "Engine.h"

#include "Core/Vec3.h"
#include "Core/Matrix.h"

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "GraphicsChip.h"
#include "Shapes.h"
#include "Model.h"

// This defines a macro called min somehow? We should avoid it at all costs and include it last
#include <SDL_syswm.h>

#include <format>

#include <SDL.h>

void RecursiveTransformTree(Node* pTreeBase)
{
	for (Node* pChild : pTreeBase->m_children)
	{
		Matrixf childLocalTrans = Matrixf::MakeTQS(pChild->m_translation, pChild->m_rotation, pChild->m_scale);
		pChild->m_worldTransform = pTreeBase->m_worldTransform * childLocalTrans;

		if (!pChild->m_children.empty())
			RecursiveTransformTree(pChild);
	}
}

void TransformNodeHeirarchy(std::vector<Node>& nodeList)
{
	std::vector<Node*> rootTransforms;

	for (Node& node : nodeList)
	{
		if (node.m_pParent == nullptr)
		{
			node.m_worldTransform = Matrixf::MakeTQS(node.m_translation, node.m_rotation, node.m_scale);

			if (!node.m_children.empty())
				rootTransforms.push_back(&node);
		}
	}

	for (Node* pRoot : rootTransforms)
	{
		RecursiveTransformTree(pRoot);
	}	
}

int main(int argc, char *argv[])
{
	{
		std::string hello = std::format("{} {}!", "hello", "world");

		int winWidth = 1280;
		int winHeight = 960;

		SDL_Window* pWindow = SDL_CreateWindow(
			"Polybox",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			int(winWidth),
			int(winHeight),
			SDL_WINDOW_RESIZABLE
		);

		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(pWindow, &wmInfo);
		HWND hwnd = wmInfo.info.win.window;

		bgfx::Init init;
		init.type = bgfx::RendererType::Direct3D11;
		init.platformData.ndt = NULL;
		init.platformData.nwh = wmInfo.info.win.window;

		bgfx::renderFrame();

		bgfx::init(init);
		bgfx::reset(winWidth, winHeight, BGFX_RESET_VSYNC);

		GraphicsChip gpu = GraphicsChip();
		gpu.Init();

		Scene tankScene("Assets/tank.gltf");
		TransformNodeHeirarchy(tankScene.m_nodes);

		bool gameRunning = true;
		float deltaTime = 0.016f;
	    Vec2i relativeMouseStartLocation{ Vec2i(0, 0) };
		bool isCapturingMouse = false;
		while (gameRunning)
		{
			Uint64 frameStart = SDL_GetPerformanceCounter();

			// Deal with events
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_KEYDOWN:
					{
						if (event.key.keysym.scancode == SDL_SCANCODE_TAB && event.key.keysym.mod & KMOD_LSHIFT)
						{
							isCapturingMouse = !isCapturingMouse;
							if (isCapturingMouse)
							{
								SDL_GetGlobalMouseState(&relativeMouseStartLocation.x, &relativeMouseStartLocation.y);
								SDL_SetRelativeMouseMode(SDL_TRUE);
							}
							else
							{
								SDL_SetRelativeMouseMode(SDL_FALSE);
								SDL_WarpMouseGlobal(relativeMouseStartLocation.x, relativeMouseStartLocation.y);
							}
						}
					}
					break;
				case SDL_WINDOWEVENT:
					switch (event.window.event)
					{
					case SDL_WINDOWEVENT_CLOSE:
						gameRunning = false;
						break;
					default:
						break;
					}
					break;
				case SDL_QUIT:
					gameRunning = false;
					break;
				}
			}
			bgfx::touch(0);

			gpu.MatrixMode(EMatrixMode::Projection);
			gpu.Identity();
			gpu.Perspective((float)320, (float)240, 1.0f, 20.0f, 60.0f);


			static float x = 0.12f;
			x += 0.004f;
			
			gpu.MatrixMode(EMatrixMode::View);
			gpu.Identity();
			gpu.Rotate(Vec3f(0.0f, 3.6f + sin(x) * 0.28f, 0.0f));
			gpu.Translate(Vec3f(1.f, -2.5f, 6.5f));
			
			// Setup some lights
			gpu.EnableLighting(true);
			gpu.NormalsMode(ENormalsMode::Custom);
			gpu.Ambient(Vec3f(0.4f, 0.4f, 0.4f));
			gpu.Light(0, Vec3f(-1.0f, 1.f, 0.0f), Vec3f(1.0f, 1.0f, 1.0f));

			gpu.EnableFog(true);
			gpu.SetFogStart(3.0f);
			gpu.SetFogEnd(15.0f);

			for (size_t i = 0; i < tankScene.m_nodes.size(); i++)
			{
				Node& node = tankScene.m_nodes[i];
				if (node.m_meshId == UINT32_MAX)
					continue;

				// Setup matrix for your model
				gpu.MatrixMode(EMatrixMode::Model);
				gpu.Identity();
				gpu.Translate(node.m_worldTransform.GetTranslation());
				gpu.Rotate(node.m_worldTransform.GetEulerRotation());
				gpu.Scale(node.m_worldTransform.GetScaling());
				
				Mesh& mesh = tankScene.m_meshes[node.m_meshId];
				if (mesh.m_name == "BlobShadow")
					continue; // We'll skip the mostly transparent blob shadows and do them later

				Primitive& prim = mesh.m_primitives[0];
				
				// Bind a texture for the model
				gpu.BindTexture(tankScene.m_images[prim.m_baseColorTexture].c_str());

				// Just draw your tank like an old fashioned fixed function pipeline
				gpu.BeginObject(EPrimitiveType::Triangles);
				for (size_t i = 0; i < prim.m_vertices.size(); i++)
				{
					gpu.Normal(prim.m_vertices[i].norm);
					gpu.Color(prim.m_vertices[i].col);
					gpu.TexCoord(prim.m_vertices[i].tex);
					gpu.Vertex(prim.m_vertices[i].pos);
				}
				gpu.EndObject();
			}

			for (size_t i = 0; i < tankScene.m_nodes.size(); i++)
			{
				Node& node = tankScene.m_nodes[i];
				if (node.m_meshId == UINT32_MAX)
					continue;

				// Setup matrix for your model
				gpu.MatrixMode(EMatrixMode::Model);
				gpu.Identity();
				gpu.Translate(node.m_worldTransform.GetTranslation());
				gpu.Rotate(node.m_worldTransform.GetEulerRotation());
				gpu.Scale(node.m_worldTransform.GetScaling());
				
				Mesh& mesh = tankScene.m_meshes[node.m_meshId];
				if (mesh.m_name != "BlobShadow")
					continue; // We'll skip the mostly transparent blob shadows and do them later

				Primitive& prim = mesh.m_primitives[0];
				
				// Bind a texture for the model
				gpu.BindTexture(tankScene.m_images[prim.m_baseColorTexture].c_str());

				// Just draw your tank like an old fashioned fixed function pipeline
				gpu.BeginObject(EPrimitiveType::Triangles);
				for (size_t i = 0; i < prim.m_vertices.size(); i++)
				{
					gpu.Normal(prim.m_vertices[i].norm);
					gpu.Color(prim.m_vertices[i].col);
					gpu.TexCoord(prim.m_vertices[i].tex);
					gpu.Vertex(prim.m_vertices[i].pos);
				}
				gpu.EndObject();
			}
		
			gpu.DrawFrame((float)winWidth, (float)winHeight);

			//bgfx::setDebug(BGFX_DEBUG_STATS);
			bgfx::frame();

			deltaTime = float(SDL_GetPerformanceCounter() - frameStart) / SDL_GetPerformanceFrequency();
		}
	}
	bgfx::shutdown();

	return 0;
}