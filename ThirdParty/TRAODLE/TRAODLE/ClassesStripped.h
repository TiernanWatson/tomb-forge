#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include "windows.h"

#define MAX2 256

class AOD_IO_CLASS;
class AE_IO_CLASS;

extern AOD_IO_CLASS AOD_IO;
extern AE_IO_CLASS AE_IO;

enum class AEFileType { LVC, TXS, AST, MDC, SEQ, CLU, AWAD, FNT, LINK, UNKNOWN };
enum class AoDFileType { CAL, CHR, CSS, TXT, CAM, CBH, CLN, EVX, POS, RMX, SCX, TMS, TMT, XXX, ZONE, UNKNOWN };
enum class BMPType { RGBA, BGRA, RGB, BGR, GRAYSCALE };
enum class TGAType { RGBA, BGRA, RGB, BGR, GRAYSCALE };
enum class DDSType { DXT1, DXT3, DXT5 };
/*enum class AoDMaterialType {NONE,
							DIFFUSE,
							BUMP,
							DIFFUSEBUMP,
							LIGHT,
							DIFFUSELIGHT,
							BUMPLIGHT,
							DIFFUSEBUMPLIGHT,
							DIFFUSEENVELOPE,
							DIFFUSEBUMPENVELOPE,
							DIFFUSEENVELOPELIGHT,
							DIFFUSEBUMPENVELOPELIGHT,
							FUR,
							GREEN,
							GLOW,
							SNOW,
							GLASS,
							DIFFUSESPECULAR,
							DIFFUSEBUMPSPECULAR,
							DIFFUSESPECULARENVELOPE,
							DIFFUSEBUMPSPECULARENVELOPE,
							IRIDESCENCE,
							HAIR,
							METAL,
							SKIN};*/


class AOD_IO_CLASS
{
private:
	class GMXlist
	{
	public:
		std::string name;						// Nome + estensione
		AoDFileType type;					// Tipologia di file
	};

public:
	TCHAR Console_OldTitle[MAX_PATH];
	LPCWSTR Console_NewTitle = L"Tomb Raider - The Angel of Darkness Level Exporter  (by Nakamichi680)";
	LPWSTR folder_exe_lpwstr = new TCHAR[MAX2];			// Cartella in cui si trova il programma
	LPWSTR folder_clzgmx_lpwstr = new TCHAR[MAX2];		// Cartella in cui si trovano i file CLZ/GMX
	std::string folder_clzgmx;								// Cartella in cui si trovano i file CLZ/GMX con backslash \ alla fine del nome (non usare per cambiare cartella)
	LPWSTR folder_level_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno spacchettati i file del livello (es. \PARIS1)
	std::string folder_level;								// Cartella in cui verranno spacchettati i file del livello (es. \PARIS1\) con backslash \ alla fine del nome
	LPWSTR folder_cameras_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno salvate le telecamere
	std::string folder_cameras;								// Cartella in cui verranno salvate le telecamere con backslash \ alla fine del nome
	LPWSTR folder_animations_lpwstr = new TCHAR[MAX2];	// Cartella in cui verranno salvate le animazioni
	std::string folder_animations;							// Cartella in cui verranno salvate le animazioni con backslash \ alla fine del nome
	LPWSTR folder_zones_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno salvate le zone
	std::string folder_zones;								// Cartella in cui verranno salvate le zone con backslash \ alla fine del nome
	LPWSTR folder_rooms_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno salvate le stanze (file RMX)
	std::string folder_rooms;								// Cartella in cui verranno salvate le stanze (file RMX) con backslash \ alla fine del nome
	LPWSTR folder_blendshapes_lpwstr = new TCHAR[MAX2];	// Cartella in cui verranno salvati i blendshapes
	std::string folder_blendshapes;							// Cartella in cui verranno salvati i blendshapes con backslash \ alla fine del nome
	LPWSTR folder_collisions_lpwstr = new TCHAR[MAX2];	// Cartella in cui verranno salvate le collisioni
	std::string folder_collisions;							// Cartella in cui verranno salvate le collisioni con backslash \ alla fine del nome
	LPWSTR folder_characters_lpwstr = new TCHAR[MAX2];	// Cartella in cui verranno salvati i personaggi
	std::string folder_characters;							// Cartella in cui verranno salvati i personaggi con backslash \ alla fine del nome
	std::string file_clzgmx;									// Nome del file del livello (può avere estensione GMX o CLZ, es. PARIS1.GMX o PARIS1.GMX.CLZ)
	std::string levelname;									// Nome del livello (es. PARIS1)
	LPWSTR folder_temp_lpwstr = new TCHAR[MAX2];			// Cartella temporanea di lavoro per le varie subroutines
	std::string folder_temp;									// Cartella temporanea di lavoro per le varie subroutines con backslash \ alla fine del nome
	std::vector <GMXlist> gmxfiles;							// Vettore contenente la lista di file esportati dal GMX

	std::string AddFileToGMXList(std::string name, AoDFileType type)		// Aggiunge un file alla lista dei file contenuti nel GMX e ritorna il nome del file in ingresso
	{
		GMXlist temp{ name, type };
		gmxfiles.push_back(temp);
		return name;
	}

	bool SearchFileInGMXList(std::string name, AoDFileType type)	// Verifica se il file o il tipo di file è presente nell'elenco dei files estratti dal GMX
	{
		GMXlist temp{ name, type };
		std::vector<GMXlist>::iterator it = find_if(gmxfiles.begin(), gmxfiles.end(), [&temp](const GMXlist& current) { return current.name == temp.name || current.type == temp.type; });
		return (it != gmxfiles.end());
	}

	AOD_IO_CLASS()												// Constructor
	{
		GetConsoleTitle(AOD_IO.Console_OldTitle, MAX_PATH);
	}

	/*LPWSTR folder_ani_lpwstr () {
		LPWSTR temp_lpwstr = new TCHAR[MAX2];
		std::string temp = fbx_folder;
		temp.append(chr_name);
		temp.append("_ANIMATIONS");
		mbstowcs(temp_lpwstr, temp.c_str(), MAX);
		return temp_lpwstr;
	};
	std::string filename_FBX_CAL (char animation_name[64]) {
		std::string temp = animation_name;
		temp.append(".FBX");
		return temp;
	};
	std::string filename_FBX_CHR () {
		std::string temp = chr_name;
		temp.append(".FBX");
		return temp;
	};
	std::string filename_FBX_TMT () {
		std::string temp = chr_name;
		temp.append("_BLENDSHAPES.FBX");
		return temp;
	};
	std::string chr_name;
	std::string CHR;											// std::stringa contenente il nome del file CHR con estensione
	std::string CAL;											// std::stringa contenente il nome del file CAL con estensione
	std::string TMT;											// std::stringa contenente il nome del file TMT con estensione
	*/
};



class AE_IO_CLASS
{
private:
	class CLUSTERlist
	{
	public:
		std::string name;						// Nome + estensione oppure hash
		AEFileType type;					// Tipologia di file
	};

public:
	TCHAR Console_OldTitle[MAX_PATH];
	LPCWSTR Console_NewTitle = L"Tomb Raider - Anniversary Edition Level Exporter";
	LPWSTR folder_exe_lpwstr = new TCHAR[MAX2];			// Cartella in cui si trova il programma
	LPWSTR folder_cluster_lpwstr = new TCHAR[MAX2];		// Cartella in cui si trova il file cluster
	std::string folder_cluster;								// Cartella in cui si trova il file cluster con backslash \ alla fine del nome (non usare per cambiare cartella)
	LPWSTR folder_level_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno spacchettati i file del livello (es. \LEVEL3A)
	std::string folder_level;								// Cartella in cui verranno spacchettati i file del livello (es. \LEVEL3A\) con backslash \ alla fine del nome
	LPWSTR folder_textures_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno salvate le textures
	std::string folder_textures;								// Cartella in cui verranno salvate le textures con backslash \ alla fine del nome
	LPWSTR folder_geometry_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno salvate le mesh del livello
	std::string folder_geometry;								// Cartella in cui verranno salvate le mesh del livello con backslash \ alla fine del nome
	LPWSTR folder_models_lpwstr = new TCHAR[MAX2];		// Cartella in cui verranno salvate le mesh dei modelli degli oggetti
	std::string folder_models;								// Cartella in cui verranno salvate le mesh dei modelli degli oggetti con backslash \ alla fine del nome
	std::string file_cluster;								// Nome del file del livello (può avere estensione PS, PC o PSP)
	std::string levelname;									// Nome del livello (es. LEVEL3A)
	LPWSTR folder_temp_lpwstr = new TCHAR[MAX2];			// Cartella temporanea di lavoro per le varie subroutines
	std::string folder_temp;									// Cartella temporanea di lavoro per le varie subroutines con backslash \ alla fine del nome
	std::vector <CLUSTERlist> clusterfiles;					// Vettore contenente la lista di file esportati dal file cluster

	std::string AddFileToClusterList(std::string name, AEFileType type)	// Aggiunge un file alla lista dei file contenuti nel file cluster e ritorna il nome del file in ingresso
	{
		CLUSTERlist temp{ name, type };
		clusterfiles.push_back(temp);
		return name;
	}

	bool SearchFileInClusterList(std::string name, AEFileType type)	// Verifica se il file o il tipo di file è presente nell'elenco dei files estratti dal file cluster
	{
		CLUSTERlist temp{ name, type };
		std::vector<CLUSTERlist>::iterator it = find_if(clusterfiles.begin(), clusterfiles.end(), [&temp](const CLUSTERlist& current) { return current.name == temp.name || current.type == temp.type; });
		return (it != clusterfiles.end());
	}

	AE_IO_CLASS()												// Constructor
	{
		GetConsoleTitle(AE_IO.Console_OldTitle, MAX_PATH);
	}
};


class Face
{
public:
	unsigned char TrisOrQuads = 3;		// Di default è impostato su "Tris"
	int v1;
	int v2;
	int v3;
	int v4;
};


class XYZ
{
public:
	float x = 0;
	float y = 0;
	float z = 0;

};


class XYZW
{
public:
	float x = 0;
	float y = 0;
	float z = 0;
	float w = 1;
};


class matrix4x4
{
public:
	float row1[4] = { 1, 0, 0, 0 };
	float row2[4] = { 0, 1, 0, 0 };
	float row3[4] = { 0, 0, 1, 0 };
	float row4[4] = { 0, 0, 0, 1 };
};


class MA_Face
{
public:
	unsigned char TrisOrQuads = 3;		// Di default è impostato su "Tris"
	int e1;
	int e2;
	int e3;
	int e4;
};


class Edge
{
public:
	int v1;
	int v2;
};


class TextureHeader
{
public:
	unsigned int Number;								// Numero della texture
	int DXT;											// 0 = ARGB, 1 = DXT1, 3 = DXT3, 5 = DXT5
	int ColourBumpShadow;								// 1 = shadow map, 2 = colour map, 4 = bump map, 5 = fur
};


class RoomInfo
{
public:
	unsigned int RoomHash;
	float tX;
	float tY;
	float tZ;
};


class MESH1_CLASS
{
public:
	std::string name;										// Nome originale (hash) o ricavato da Get_mesh_name (usato per Null)
	bool hashed;										// True se "name" è un hash
	unsigned int nElements;								// Il numero di elementi del gruppo MESH1
	std::vector <float> X;
	std::vector <float> Y;
	std::vector <float> Z;
	std::vector <float> Xn;
	std::vector <float> Yn;
	std::vector <float> Zn;
	std::vector <float> Xtg;
	std::vector <float> Ytg;
	std::vector <float> Ztg;
	std::vector <float> Xbn;
	std::vector <float> Ybn;
	std::vector <float> Zbn;
	std::vector <float> U;
	std::vector <float> V;
	std::vector <float> W1;
	std::vector <float> W2;
	std::vector <uint8_t> Bone1;
	std::vector <uint8_t> Bone2;
	std::vector <unsigned short> arrIndex;					// Contiene lo strip
	std::vector <unsigned short> arrEl_Ind;					// Contiene il numero di indici dell'elemento
	std::vector <unsigned short> arrOffset;					// Contiene l'offset dell'elemento nel triangle strip
	std::vector <unsigned short> arrMat_Ref;					// ID materiale
	std::vector <unsigned short> arrDraw_Mode;				// Draw mode (4 o 5)
	std::string name_lower()
	{								// Restituisce lo stesso nome di "name" ma in minuscolo (usato per Mesh)
		std::string out = name;
		transform(out.begin(), out.end(), out.begin(), ::tolower);
		return out;
	};
};


class MESH2_CLASS
{
public:
	std::string name;										// Nome originale (hash) o ricavato da Get_mesh_name (usato per Null)
	bool hashed;										// True se "name" è un hash
	int Bone;											// La bone a cui è associato il gruppo MESH2
	unsigned int nV;									// Il numero di vertici del gruppo MESH2
	unsigned int nElements;								// Il numero di elementi del gruppo MESH2
	std::vector <float> X;
	std::vector <float> Y;
	std::vector <float> Z;
	std::vector <float> Xn;
	std::vector <float> Yn;
	std::vector <float> Zn;
	std::vector <float> Xtg;
	std::vector <float> Ytg;
	std::vector <float> Ztg;
	std::vector <float> Xbn;
	std::vector <float> Ybn;
	std::vector <float> Zbn;
	std::vector <float> U;
	std::vector <float> V;
	std::vector <unsigned short> arrIndex;					// Contiene lo strip
	std::vector <unsigned short> arrEl_Ind;					// Contiene il numero di indici dell'elemento
	std::vector <unsigned short> arrOffset;					// Contiene l'offset dell'elemento nel triangle strip
	std::vector <unsigned short> arrMat_Ref;					// ID materiale
	std::vector <unsigned short> arrDraw_Mode;				// Draw mode (4 o 5)
	std::string name_lower()
	{								// Restituisce lo stesso nome di "name" ma in minuscolo (usato per Mesh)
		std::string out = name;
		transform(out.begin(), out.end(), out.begin(), ::tolower);
		return out;
	};
};


class TMT_CLASS
{
public:
	std::string name;										// Nome originale (hash) o ricavato da Get_mesh_name (usato per Null)
	bool hashed;										// True se "name" è un hash
	unsigned int nBlendshapes = 0;						// Numero di blendshapes presenti (a cui va aggiunto 1 per la mesh normale)
	unsigned int nV;									// Numero di vertici del blendshape
	unsigned int mesh2_group;							// Indice del gruppo MESH 2 a cui il blendshape è associato
	unsigned int mesh2_bone;							// Indice della bone dello scheletro associata al blendshape
	//Material TMT_Material;								// Il materiale del TMT è forzato a "Diffuse" (4)
	std::vector < std::vector <int> > BLENDSHAPE_vINDX;			// Contiene gli indici dei vertici diversi dalla mesh normale di ogni blendshape (per file FBX)		
	std::vector < std::vector <float> > X;
	std::vector < std::vector <float> > Y;
	std::vector < std::vector <float> > Z;
	std::vector < std::vector <float> > Xn;
	std::vector < std::vector <float> > Yn;
	std::vector < std::vector <float> > Zn;
	std::vector < std::vector <float> > U;
	std::vector < std::vector <float> > V;
	std::string name_lower()
	{								// Restituisce lo stesso nome di "name" ma in minuscolo (usato per Mesh)
		std::string out = name;
		transform(out.begin(), out.end(), out.begin(), ::tolower);
		return out;
	};
};





// WIP


class Material
{
public:
	enum class TYPE { EMPTY, LAMBERT, PHONG, BLINN, AISTANDARDSURFACE };
	enum class BLEND { NORMAL, SUBTRACT, OVERLAY };
	enum class SPECIAL { NO, FUR, GLOW, SNOW, GLASS };
	std::string name;										// Nome del materiale
	TYPE Type = TYPE::EMPTY;							// Tipologia di shader
	BLEND Blend = BLEND::NORMAL;						// Tipologia di blending
	SPECIAL Special = SPECIAL::NO;						// Materiale speciale, valore di default = NO
	std::string color;										// Texture diffuse
	std::string shadow;										// Texture shadow map
	std::string transparency;								// Texture trasparenza
	std::string bump;										// Texture bump map
	std::string specular;									// Texture specular/envelope
	std::string emissive;									// Texture per effetto glow
	float metalness = 0.3f;
	float specular_color_R = 1;							// Colore riflesso 0-1
	float specular_color_G = 1;							// Colore riflesso 0-1
	float specular_color_B = 1;							// Colore riflesso 0-1
	float opacity_R = 1;								// Valore trasparenza 0-1
	float opacity_G = 1;								// Valore trasparenza 0-1
	float opacity_B = 1;								// Valore trasparenza 0-1
};

/*class Material
{
public:
	unsigned int Number;								// Numero del materiale
	short Type;											// Tipologia materiale (None, diffuse, bump, specular, ecc.)
	short DoubleSide;
	int Diffuse;										// ID texture slot 1
	int Shadow;											// ID texture slot 2
	int BumpSpec;										// ID texture slot 3
	int Fur;											// ID texture slot 4
};*/








///////////// CLASSI BUONE
class Joint
{
public:
	enum class LABELSIDE { CENTER, LEFT, RIGHT, NONE };
	enum class LABELTYPE { NONE, ROOT, HIP, KNEE, FOOT, TOE, SPINE, NECK, HEAD, COLLAR, SHOULDER, ELBOW, HAND, FINGER, THUMB, PROPA, PROPB, PROPC, OTHER, INDEX_FINGER, MIDDLE_FINGER, RING_FINGER, PINKY_FINGER, EXTRA_FINGER, BIG_TOE, INDEX_TOE, MIDDLE_TOE, RING_TOE, PINKY_TOE, FOOT_THUMB };
	std::string name;
	std::string parent;						// Nome del gruppo/oggetto da cui dipende
	std::string FBX_parent = "0";			// hashID del gruppo/oggetto da cui dipende
	std::string layer;						// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
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
	float Radius = 20;					// Raggio sfera joint. Nel file FBX va moltiplicato per 33
	bool InheritsTransform = true;
	bool DrawLabel = false;
	LABELSIDE LabelSide = LABELSIDE::CENTER;
	LABELTYPE LabelType = LABELTYPE::OTHER;
	std::string LabelName = "";				// Il LabelName viene usato solo LabelType è su Other
};


class Texture
{
private:
	class uvChooser
	{
	public:
		unsigned int UVset;						// 1, 2, 3, 4... (l'UVset 0 non va inserito, non è necessario l'uvChooser). MA Exporter al momento supporta solo UVset 0 e 1
		std::string mesh_name;
	};

public:
	std::string name;								// Nome dell'oggetto texture (ad esempio "PRAGUE3_ZONE2_TEXTURE5")
	std::string filename;							// Nome e percorso del file della texture (ad esempio "D:\AoD\C++\PARIS2_1_Z00_34.dds")
	std::vector <uvChooser> UV_mesh_list;			// Aggiungere solo le mesh con UVset diverso da 0
	float Exposure = 0;
	bool AlphaIsLuminance = false;

	int UVChannel = 1;	// 1 o 2. DA RIMUOVERE!!!!!!
};


class Locator
{
public:
	std::string name;						// Nome dell'oggetto locator (ad esempio "PRAGUE3_ROOM_5_NODE_2")
	std::string parent;						// Nome del gruppo/oggetto da cui dipende
	std::string FBX_parent = "0";			// hashID del gruppo/oggetto da cui dipende
	std::string layer;						// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
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
};


class Light
{
public:
	std::string name;						// Nome della luce (ad esempio "PRAGUE3_ROOM_5_LIGHT_2")
	std::string parent;						// Nome del gruppo/oggetto da cui dipende
	std::string FBX_parent = "0";			// hashID del gruppo/oggetto da cui dipende
	std::string layer;						// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
	std::string type = "Point";				// Tipo di luce (Point/Ambient/Spot)
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
	float Intensity = 100;				// Intensità luce
	float R = 1;						// Intensità colore rosso (0-1)
	float G = 1;						// Intensità colore verde (0-1)
	float B = 1;						// Intensità colore blu (0-1)
	float Decay_Near_Start = 0;			// Di regola è sempre 0 in AoD
	float Decay_Near_End = 0;			// Per luce più diffusa in Arnolds impostare come Attenuation_Far_End
	float Decay_Far_Start = 0;			// Dove la luce inizia a sbiadirsi
	float Decay_Far_End = 0;			// Distanza massima raggiunta dalla sfera di luce
};



class Camera
{
public:
	std::string name;					// Nome della telecamera (ad esempio "IG_9_15")
	std::string parent;					// Nome del gruppo/oggetto da cui dipende
	std::string FBX_parent = "0";		// hashID del gruppo/oggetto da cui dipende
	std::string layer;					// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
	bool translate_flag = false;
	bool rotate_flag = false;
	bool scale_flag = false;
	float tX = 0;					// Traslazione X
	float tY = 0;					// Traslazione Y
	float tZ = 0;					// Traslazione Z
	float rX = 0;					// Rotazione X
	float rY = 0;					// Rotazione Y
	float rZ = 0;					// Rotazione Z
	float sX = 1;					// Scalatura X
	float sY = 1;					// Scalatura Y
	float sZ = 1;					// Scalatura Z
	float hfa = 2.367f;				// Horizontal Film Aperture
	float vfa = 1.33f;				// Vertical Film Aperture
	float fl = 35;					// Focal Lenght
	float coi = 45.453f;			// Center Of Interest
	float ncp = 10;					// Near clip plane
	float fcp = 1000000;			// Far clip plane
};


class Transform
{
public:
	std::string name;					// Nome del gruppo
	std::string parent;					// Nome del gruppo/oggetto da cui dipende 
	std::string FBX_parent = "0";		// hashID del gruppo/oggetto da cui dipende
	std::string layer;					// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
	bool translate_flag = false;
	bool rotate_flag = false;
	bool scale_flag = false;
	float tX = 0;					// Traslazione X
	float tY = 0;					// Traslazione Y
	float tZ = 0;					// Traslazione Z
	float rX = 0;					// Rotazione X
	float rY = 0;					// Rotazione Y
	float rZ = 0;					// Rotazione Z
	float sX = 1;					// Scalatura X
	float sY = 1;					// Scalatura Y
	float sZ = 1;					// Scalatura Z
};


class Mesh
{
public:
	std::string name;					// Nome del modello geometrico
	std::string parent;					// Nome del gruppo/oggetto da cui dipende
	std::string FBX_parent = "0";		// hashID del gruppo/oggetto da cui dipende
	std::string layer;					// Nome layer di appartenenza, se vuoto non appartiene ad alcun layer (solo file Maya ASCII)
	std::string material_name;			// Nome del materiale associato alla mesh
	unsigned int nV;
	bool uv_set1_flag = true;
	bool uv_set2_flag = true;
	bool normals_flag = true;
	bool tangents_flag = true;
	bool binormals_flag = true;
	bool vcolors_flag = true;
	bool doublesided = true;
	std::vector <float> X;
	std::vector <float> Y;
	std::vector <float> Z;
	std::vector <float> U1;
	std::vector <float> V1;
	std::vector <float> U2;
	std::vector <float> V2;
	std::vector <float> Xn;
	std::vector <float> Yn;
	std::vector <float> Zn;
	std::vector <float> Xtg;
	std::vector <float> Ytg;
	std::vector <float> Ztg;
	std::vector <float> Xbn;
	std::vector <float> Ybn;
	std::vector <float> Zbn;
	std::vector <float> R;
	std::vector <float> G;
	std::vector <float> B;
	std::vector <float> A;
	std::vector <Face> Face;		// utilizzato da FBX (tris e quads) e MA (solo tris)
};