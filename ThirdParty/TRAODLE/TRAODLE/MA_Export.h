#pragma once

#include <vector>
#include <sstream>
#include "ClassesStripped.h"

enum class LayerDisplayType
{
	Normal,			// L'oggetto è renderizzato e può essere selezionato
	Reference,		// L'oggetto è renderizzato e non può essere selezionato
	Template		// L'oggetto è in wireframe e non può essere selezionato
};


class Layer
{
public:
	std::string name;											// Nome del layer
	unsigned int Label_ARGB = 0xFFFFFFFF;					// Colore etichetta (la trasparenza viene ignorata)
	bool Visible = true;									// Controlla la visibilità del layer
	LayerDisplayType Type = LayerDisplayType::Normal;		// Controlla se la selezione del layer è bloccata						
};


class NurbsSurface
{
public:
	std::string name;					// Nome del nurbs
	std::string parent;					// Nome del gruppo/oggetto da cui dipende
	std::string layer;					// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
	std::string Type;					// Tipologia di nurbs
	bool translate_flag = false;
	bool rotate_flag = false;
	bool scale_flag = false;
	float tX = 0;						// Traslazione X
	float tY = 0;						// Traslazione Y
	float tZ = 0;						// Traslazione Z
	float rX = 0;						// Rotazione X
	float rY = 0;						// Rotazione Y
	float rZ = 0;						// Rotazione Z
	float sX = 1;						// Scalatura X
	float sY = 1;						// Scalatura Y
	float sZ = 1;						// Scalatura Z
	unsigned int Sections = 8;			// Numero di settori (linee divisorie verticali)
	unsigned int Spans = 6;				// Numero di spans (linee divisorie orizzontali)
	float Radius = 1;					// Raggio della sfera
};


class PolyPlane
{
public:
	std::string name;
	std::string parent;						// Nome del gruppo/oggetto da cui dipende
	std::string layer;						// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
	std::string material_name;				// Nome del materiale associato al PolyPlane
	bool translate_flag = false;
	bool rotate_flag = false;
	bool scale_flag = false;
	bool rotate_pivot_flag = false;
	bool scale_pivot_flag = false;
	bool Visible = true;
	float tX = 0;						// Traslazione X
	float tY = 0;						// Traslazione Y
	float tZ = 0;						// Traslazione Z
	float rX = 0;						// Rotazione X
	float rY = 0;						// Rotazione Y
	float rZ = 0;						// Rotazione Z
	float sX = 1;						// Scalatura X
	float sY = 1;						// Scalatura Y
	float sZ = 1;						// Scalatura Z
	float rpX = 0;						// Rotate pivot X
	float rpY = 0;						// Rotate pivot Y
	float rpZ = 0;						// Rotate pivot Z
	float spX = 0;						// Scale pivot X
	float spY = 0;						// Scale pivot Y
	float spZ = 0;						// Scale pivot Z
	float Width = 10;					// Larghezza
	float Height = 10;					// Altezza
	unsigned int Subdiv_Width = 10;		// Numero suddivisioni larghezza
	unsigned int Subdiv_Height = 10;	// Numero suddivisioni altezza
};


class BossWave
{
public:
	std::string name;
	std::string parent;						// Nome del gruppo/oggetto da cui dipende
	std::string layer;						// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
	std::string inputmesh_name;				// Nome della mesh a cui viene applicata l'onda
	std::string material_name;				// Nome del materiale associato alla BossWave
	bool translate_flag = false;
	bool rotate_flag = false;
	bool scale_flag = false;
	bool rotate_pivot_flag = false;
	bool scale_pivot_flag = false;
	float tX = 0;						// Traslazione X
	float tY = 0;						// Traslazione Y
	float tZ = 0;						// Traslazione Z
	float rX = 0;						// Rotazione X
	float rY = 0;						// Rotazione Y
	float rZ = 0;						// Rotazione Z
	float sX = 1;						// Scalatura X
	float sY = 1;						// Scalatura Y
	float sZ = 1;						// Scalatura Z
	float rpX = 0;						// Rotate pivot X
	float rpY = 0;						// Rotate pivot Y
	float rpZ = 0;						// Rotate pivot Z
	float spX = 0;						// Scale pivot X
	float spY = 0;						// Scale pivot Y
	float spZ = 0;						// Scale pivot Z
	float patchSizeX = 200;
	float patchSizeZ = 200;
	float spaceScale = 1;
	float waveHeight = 1;
	float windSpeed = 4;
	float oceanDepth = 10000;
};


class MA_KeyTimeValue
{
public:
	std::vector <unsigned int> Time;
	std::vector <float> Value;
};


class MA_animCurve
{
public:
	std::string name;
	std::string parent;
	unsigned int nFrames;
	bool translateX_flag = false;
	bool translateY_flag = false;
	bool translateZ_flag = false;
	bool rotateX_flag = false;
	bool rotateY_flag = false;
	bool rotateZ_flag = false;
	bool scaleX_flag = false;
	bool scaleY_flag = false;
	bool scaleZ_flag = false;
	bool focalLength_flag = false;
	bool centerOfInterest_flag = false;
	MA_KeyTimeValue tX;
	MA_KeyTimeValue tY;
	MA_KeyTimeValue tZ;
	MA_KeyTimeValue rX;
	MA_KeyTimeValue rY;
	MA_KeyTimeValue rZ;
	MA_KeyTimeValue sX;
	MA_KeyTimeValue sY;
	MA_KeyTimeValue sZ;
	MA_KeyTimeValue fl;
	MA_KeyTimeValue coi;
};

class MA_EXPORT
{
public:
	char UpAxis = 'z';
	float NearClipPlane = 2;
	float FarClipPlane = 1000000;
	std::vector <Texture> Texture;
	std::vector <Material> Material;
	std::vector <Layer> Layer;
	std::vector <Transform> Transform;
	std::vector <Mesh> Mesh;
	std::vector <Light> Light;
	std::vector <Camera> Camera;
	std::vector <Locator> Locator;
	std::vector <NurbsSurface> NurbsSurface;
	std::vector <Joint> Joint;
	std::vector < std::vector <MA_animCurve> > Animation;
	std::vector <PolyPlane> PolyPlane;
	std::vector <BossWave> BossWave;

	std::stringstream MA_Header;
	std::stringstream MA_Nodes;
	std::stringstream MA_Select;
	std::stringstream MA_Connections;
	std::stringstream MA_Relationships;
};