#define BUILD_GRAPHICS
#ifdef BUILD_GRAPHICS

#ifndef GRAPHICS_H
#define GRAPHICS_H


#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#define GRID_SIZE 64.0

typedef SDL_Surface Screen;
typedef SDL_Rect Rectangle;
typedef short unsigned int SUint;
typedef unsigned char Uint8;

#include <vector>
#include <list>



#include <fstream>




class Texture;
class Font;



class Graphics
{
	public:

	class Colour
	{
		public:
			Colour(float red=0, float green=0, float blue=0, float alpha=0) : r(red), g(green), b(blue), a(alpha) {}
			Colour(const Colour & cpy) : r(cpy.r), g(cpy.g), b(cpy.b), a(cpy.a) {}
	
			Colour & operator=(const Colour & s) {r = s.r; g = s.g; b = s.b; a = s.a; return *this;}
			bool operator==(const Colour & s) const {return (r == s.r && g == s.g && b == s.b && a == s.a);}
			bool operator!=(const Colour & s) const {return !operator==(s);}
			float r;
			float g;
			float b;
			float a;
	
	};
		static int ScreenWidth() {return screenWidth;}
		static int ScreenHeight() {return screenHeight;}

		static void Initialise(const char * caption, int width=640, int height=480);
		static void Destroy();

		static SDL_Surface * LoadTextureBMP(const char * file);
		static void SaveTextureBMP(SDL_Surface * tex, const char * file);
		
		static void ClearScreen();
		static void UpdateScreen();
		static void DrawGrid(int gridWidth, int gridHeight, Colour colour);
		static Uint8 MakeColour(int R, int G, int B, int Alpha = 0);
		static Colour ConvertColour(Uint8 colour);

		static void DrawTexture(SDL_Surface * src, int destX, int destY, int srcX=0, int srcY=0, int width=-1, int height=-1);
		static void DrawTexture(SDL_Surface * src, int destX, int destY, double rotate, double scale);
		static void DrawPixel(int x, int y, Colour colour);

		static Colour GetPixel(int x, int y);
		static void DrawLine(int x1, int y1, int x2, int y2, Colour colour, double scale=1.0);
		static void DrawLineDashed(int x1, int y1, int x2, int y2, Colour colour, double scale=1.0);
		static void DrawRectangle(int x1, int y1, int x2, int y2, Colour colour, double scale=1.0);

		static void GetColourData(SDL_Surface * src, std::vector<SUint> * R, std::vector<SUint> * G, std::vector<SUint> * B, std::vector<SUint> * A = NULL);
		static void GetColourData(SDL_Surface * src, std::vector<std::vector<SUint> > * R, std::vector<std::vector<SUint> > * G,  std::vector<std::vector<SUint> > * B,  std::vector<std::vector<SUint> > * A = NULL);

		static void DrawColourData(int destX, int destY, std::vector<SUint> * R, std::vector<SUint> * G, std::vector<SUint> * B, std::vector<SUint> * A = NULL) {DrawColourData(screen, destX, destY, R, G, B, A);}

		static void DrawColourData(int destX, int destY, std::vector<std::vector<SUint> > * R, std::vector<std::vector<SUint> > * G, std::vector<std::vector<SUint> > * B, std::vector<std::vector<SUint> > * A = NULL) {DrawColourData(screen, destX, destY, R, G, B, A);}

		static SDL_Surface * TextureFromColours(std::vector<SUint> * R, std::vector<SUint> * G, std::vector<SUint> * B, std::vector<SUint> * A = NULL);
		static SDL_Surface * TextureFromColours(std::vector<std::vector<SUint> > * R, std::vector<std::vector<SUint> > * G, std::vector<std::vector<SUint> > * B, std::vector<std::vector<SUint> > * A = NULL);
		
		static void Wait(int n);

		template <class T>
		class TextureManager
		{
			public:
				TextureManager() {}
				virtual ~TextureManager() {}

				virtual Texture & operator[](const T & at) = 0;
		};

		static bool Initialised() {return initialised;}
		static void ScreenShot(const char * fileName);
	protected:
		static void DrawColourData(SDL_Surface * dest, int destX, int destY, std::vector<SUint> * R, std::vector<SUint> * G, std::vector<SUint> * B, std::vector<SUint> * A = NULL);
		static void DrawColourData(SDL_Surface * dest, int destX, int destY, std::vector<std::vector<SUint> > * R, std::vector<std::vector<SUint> > * G, std::vector<std::vector<SUint> > * B, std::vector<std::vector<SUint> > * A = NULL);
		static void DrawTexture(SDL_Surface * dest, SDL_Surface * src, int srcX, int srcY, int destX, int destY, int width, int height);
		static void DrawPixel(SDL_Surface * dest, int x, int y, Colour colour);
		static Colour GetPixel(SDL_Surface * dest, int x, int y);
		static void DrawLine(SDL_Surface * dest, int x1, int y1, int x2, int y2, Colour colour);

	private:
		Graphics() {}
		~Graphics() {}	
		static std::list<SDL_Surface*> allTextures;
		static Screen * screen;

		static int screenWidth;
		static int screenHeight;
		static bool initialised;
		

};
typedef Graphics::Colour Colour;

class Texture
{
	public:
		Texture(const char * fileName, bool newDrawCentred = true);
		virtual ~Texture();

		void Draw(int x, int y, double angle=0, double scale=1);
		void DrawColour(int x, int y, double angle, double scale, Colour colour);
		
		int width() const {return surface->w;}
		int height() const {return surface->h;}

	protected:
		SDL_Surface * surface;
		GLuint texture;

	private:
		bool drawCentred;
		
};	

class Font : private Texture
{
	public:
		Font(const char * fileName, int newWidth, int newHeight);
		virtual ~Font();

		void DrawTextColour(const char * string, int x, int y, double angle, double scale, Colour colour);
		void DrawText(const char * string, int x, int y, double angle=0, double scale=1);	
	private:
		int width;
		int height;
};




#endif //GRAPHICS_H

#endif //BUILD_GRAPHICS

//EOF
