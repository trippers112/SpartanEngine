/*
Copyright(c) 2016-2019 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES ======================
#include "IComponent.h"
#include <vector>
#include "../../Math/BoundingBox.h"
#include "../../Math/Matrix.h"
//=================================

namespace Spartan
{
	class Model;
	class Mesh;
	class Light;
	class Material;
	namespace Math
	{
		class Vector3;
	}

	enum Geometry_Type
	{
		Geometry_Custom,
		Geometry_Default_Cube,
		Geometry_Default_Quad,
		Geometry_Default_Sphere,
		Geometry_Default_Cylinder,
		Geometry_Default_Cone
	};

	class SPARTAN_CLASS Renderable : public IComponent
	{
	public:
		Renderable(Context* context, Entity* entity, uint32_t id = 0);
		~Renderable() = default;

		//= ICOMPONENT ===============================
		void Serialize(FileStream* stream) override;
		void Deserialize(FileStream* stream) override;
		//============================================

		//= GEOMETRY ==========================================================================================
		void GeometrySet(
			const std::string& name,
			uint32_t index_offset,
			uint32_t index_count,
			uint32_t vertex_offset,
			uint32_t vertex_count,
			const Math::BoundingBox& aabb, 
			Model* model
		);
		void GeometryGet(std::vector<uint32_t>* indices, std::vector<RHI_Vertex_PosTexNorTan>* vertices) const;
		void GeometrySet(Geometry_Type type);
		auto GeometryIndexOffset()	const { return m_geometryIndexOffset; }
		auto GeometryIndexCount()	const { return m_geometryIndexCount; }		
		auto GeometryVertexOffset() const { return m_geometryVertexOffset; }
		auto GeometryVertexCount()	const { return m_geometryVertexCount; }
		auto GeometryType()			const { return m_geometry_type; }
		const auto& GeometryName()	const { return m_geometryName; }
		const auto& GeometryModel() const { return m_model; }
		const Math::BoundingBox& GetAabb();
		//=====================================================================================================

		//= MATERIAL ============================================================
		// Sets a material from memory (adds it to the resource cache by default)
		void MaterialSet(const std::shared_ptr<Material>& material);

		// Loads a material and the sets it
		std::shared_ptr<Material> MaterialSet(const std::string& file_path);

		void UseDefaultMaterial();
		std::string GetMaterialName();
		const auto& GetMaterial()	const { return m_material; }
		auto HasMaterial()			const { return m_material != nullptr; }
		//=======================================================================

		//= PROPERTIES ============================================================================
		void SetCastShadows(const bool cast_shadows)		{ m_castShadows = cast_shadows; }
		auto GetCastShadows() const							{ return m_castShadows; }
		void SetReceiveShadows(const bool receive_shadows)	{ m_receiveShadows = receive_shadows; }
		auto GetReceiveShadows() const						{ return m_receiveShadows; }
		//=========================================================================================

	private:
		std::string m_geometryName;
		uint32_t m_geometryIndexOffset;
		uint32_t m_geometryIndexCount;
		uint32_t m_geometryVertexOffset;
		uint32_t m_geometryVertexCount;
		std::shared_ptr<Model> m_model;
		Geometry_Type m_geometry_type;
		Math::BoundingBox m_bounding_box;
		Math::BoundingBox m_aabb;
        Math::BoundingBox m_oobb;
        Math::Matrix m_last_transform   = Math::Matrix::Identity;
        bool m_is_dirty                 = true;
        bool m_castShadows              = true;
        bool m_receiveShadows           = true;
		bool m_materialDefault;
        std::shared_ptr<Material> m_material;
	};
}
