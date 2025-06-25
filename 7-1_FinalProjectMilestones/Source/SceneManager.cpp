///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#include <glm/gtx/transform.hpp>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL mat;
	mat.tag = "default"; //set name
	mat.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f); //set diffuse color
	mat.specularColor = glm::vec3(0.5f, 0.5f, 0.5f); //set specular color
	mat.shininess = 32.0f;
	m_objectMaterials.push_back(mat); //add material to the list

	OBJECT_MATERIAL ceramic;
	ceramic.tag = "ceramicRed";
	ceramic.diffuseColor = glm::vec3(1.0f, 0.0f, 0.0f);  // deep red base
	ceramic.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // bright specular highlights
	ceramic.shininess = 64.0f;                          // sharp shiny reflections
	m_objectMaterials.push_back(ceramic);
}

void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help
	***/

	// Directional light - fluorescent white from above
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-5.0f, -10.0f, -5.0f)); // Top-left downward
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.4f, 0.4f, 0.4f));  // strong ambient
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));  // max white diffuse
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.0f, 1.0f, 1.0f)); // sharp white highlights

	// Point light - warm sunlight from the upper right
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(10.0f, 12.0f, -5.0f)); // elevated right
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.2f, 0.15f, 0.1f));   // soft warm ambient
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.8f, 0.6f, 0.4f));    // golden diffuse
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(1.0f, 0.9f, 0.8f));   // bright warm specular

}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPyramid4Mesh();

	CreateGLTexture("textures\\wood.jpg", "desk"); // for the base plane
	CreateGLTexture("textures\\whiteWall.jpg", "wall"); //for the back plane
	CreateGLTexture("textures\\matteBlack.jpg", "matteBlack"); //for the outer cylinder and handle
	CreateGLTexture("textures\\foam.jpg", "foam"); //for the top of the mug
	CreateGLTexture("textures\\pyramid.jpg", "pyramid");
	CreateGLTexture("textures\\screen.jpg", "screen");

	BindGLTextures();

	DefineObjectMaterials();

	SetupSceneLights();

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
#pragma region Desk and wall
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	//Enable Lighting
	m_pShaderManager->setIntValue(g_UseLightingName, true);
	//Set Material
	SetShaderMaterial("default");
	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("desk");
	SetTextureUVScale(1.0f, 1.0f);
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// === Back plane ===
	scaleXYZ = glm::vec3(20.0f, 1.0f, 15.0f);

	XrotationDegrees = 90.0f; //rotate to be a backdrop
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 15.0f, -15.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("wall");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
#pragma endregion


#pragma region Cup
	// === Mug Outer Cylinder ===
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.5f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("matteBlack");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// === Inner Cavity (Red Interior) ===
	scaleXYZ = glm::vec3(0.9f, 1.8f, 0.9f); // Slightly smaller

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.5f, 0.21f, 0.0f); // Slightly higher

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f); // Glossy red
	m_basicMeshes->DrawCylinderMesh();

	// === Inner Cavity (Black Interior) ===
	scaleXYZ = glm::vec3(0.8f, 1.7f, 0.8f); // Slightly smaller

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-7.5f, 0.32f, 0.0f); // Slightly higher

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Matte black and lowered transparency to mimic shadow
	m_pShaderManager->setIntValue(g_UseLightingName, false); //disable lighting
	SetShaderTexture("foam");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();


	// === Handle ===
	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f); // Size of the handle

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-8.5f, 1.0f, 0.0f); // Side of mug

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Same matte black
	SetShaderTexture("matteBlack");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawTorusMesh();
#pragma endregion

#pragma region Keyboard
	// ===Keyboard Main===
	scaleXYZ = glm::vec3(10.0f, 0.5f, 4.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 0.25f, 3.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("matteBlack");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ===Keys===
	// Key size and spacing
	glm::vec3 keyScale(0.8f, 0.2f, 0.8f);
	float spacingX = 1.0f;
	float spacingZ = 1.0f;

	// Keyboard center offset
	float startX = -4.5f; // (10.0f / spacingX - 1) / -2
	float startZ = 1.0f;  // Positioned slightly above keyboard base Z=3.0f

	std::vector<std::string> row1 = { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P" };
	std::vector<std::string> row2 = { "A", "S", "D", "F", "G", "H", "J", "K", "L" };
	std::vector<std::string> row3 = { "Z", "X", "C", "V", "B", "N", "M" };

	auto drawKeyRow = [&](std::vector<std::string> keys, float rowZ) {
		float offsetX = startX + (10 - keys.size()) * 0.5f;
		for (size_t i = 0; i < keys.size(); ++i) {
			float x = offsetX + i * spacingX;
			glm::vec3 keyPosition(x, 0.5f, startZ + rowZ);  // slightly above keyboard base Y

			SetTransformations(keyScale, 0.0f, 0.0f, 0.0f, keyPosition);
			SetShaderColor(0.83f, 0.83f, 0.83f, 1.0f); // Light grey
			m_basicMeshes->DrawBoxMesh();
		}
		};

	// Draw 3 rows of keys
	drawKeyRow(row3, 3.0f); // Z = 1 unit behind center
	drawKeyRow(row2, 2.0f);
	drawKeyRow(row1, 1.0f);
#pragma endregion


#pragma region Pyramid
	// ===Pyramid===
	scaleXYZ = glm::vec3(2.0f, 5.0f, 2.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(8.0f, 2.5f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("pyramid");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPyramid4Mesh();
#pragma endregion
	
#pragma region Computer
	// ===Computer Base==
	scaleXYZ = glm::vec3(3.0f, 0.5f, 3.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 0.5f, -4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("matteBlack");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// ===Computer Stand===
	scaleXYZ = glm::vec3(1.0f, 9.0f, 1.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 5.0f, -5.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("matteBlack");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ===Computer Stand Arm===
	scaleXYZ = glm::vec3(1.0f, 1.0f, 2.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 8.0f, -4.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("matteBlack");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ===Computer Screen ===
	scaleXYZ = glm::vec3(15.0f, 10.0f, 1.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 8.0f, -3.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("matteBlack");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// ===Screen===

	scaleXYZ = glm::vec3(6.0f, 1.0f, 4.0f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 8.0f, -2.9f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark matte black
	SetShaderTexture("screen");
	//SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();
#pragma endregion




	/****************************************************************/
}
