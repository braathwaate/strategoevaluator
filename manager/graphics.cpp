#include "graphics.h"
#include <cmath>
#include <cassert>
#include <iostream>

#ifdef BUILD_GRAPHICS

#undef DEBUG
//#define DEBUG

std::list<SDL_Surface*> Graphics::allTextures = std::list<SDL_Surface*>();
Screen * Graphics::screen = NULL;

int Graphics::screenWidth = 0;
int Graphics::screenHeight = 0;
bool Graphics::initialised = false;

using namespace std;

Texture::Texture(const char * filename, bool newDrawCentred) : surface(NULL), texture(0), drawCentred(newDrawCentred)
{
	#ifdef DEBUG
		printf("Texture::Texture - loading \"%s\".\n", filename);
	#endif //DEBUG

	surface = Graphics::LoadTextureBMP(filename);
	if (surface == NULL)
	{
		fprintf(stderr, "Texture::Texture - Could not open texture from file \"%s\"! ABORT\n", filename);
		exit(EXIT_FAILURE);
	}

	GLenum texture_format; 
	GLint nOfColours = surface->format->BytesPerPixel;
	switch (nOfColours)
	{
		case 4: //contains alpha
			texture_format = (surface->format->Rmask == 0x000000FF) ? GL_RGBA : GL_BGRA;
			break;
		case 3: //does not contain alpha
			texture_format = (surface->format->Rmask == 0x000000FF) ? GL_RGB : GL_BGR;	
			break;
		default:
			fprintf(stderr,"Texture::Texture - Could not understand SDL_Surface format (%d colours)! ABORT\n", nOfColours);
			exit(EXIT_FAILURE);
			break;	
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D(GL_TEXTURE_2D, 0, nOfColours, surface->w, surface->h,0, texture_format, GL_UNSIGNED_BYTE, surface->pixels);

}

Texture::~Texture()
{
	#ifdef DEBUG
		printf("Texture::~Texture - %p has been deleted. glDeleteTexture and SDL_FreeSurface here.\n", (void*)(this));
	#endif //DEBUG
	glDeleteTextures(1, &texture); 
	//SDL_FreeSurface(surface);
}

void Texture::DrawColour(int x, int y, double angle, double scale, Colour colour)
{
	if (scale > surface->w || scale > surface->h)
	{
		Graphics::DrawPixel(x/scale,y/scale,colour);
	}
	else
	{
		glColor4f(colour.r,colour.g,colour.b,1);	
		Draw(x,y,angle,scale);
		glColor4f(1,1,1,1);
	}
}

void Texture::Draw(int x, int y, double angle , double scale )
{
	//Draws the CENTRE of the texture at x, y, rotated by angle
	
	#ifdef DEBUG
		printf("	Texture::Draw - Drawing %p at (%d, %d) ; angle %2f ; scale % 2f\n", (void*)(this), x, y, angle, scale);
	#endif //DEBUG

	//if (x/scale < 0 || x/scale > Graphics::ScreenWidth() || y/scale < 0 || y/scale > Graphics::ScreenHeight() )
	//	return;

	glPushMatrix(); //NOT deprecated
	
	
	glTranslatef(x/scale, y/scale,0);

	if (scale > surface->w || scale > surface->h)
	{
		Graphics::DrawPixel(0,0, Colour(255,255,255));
	}
	else
	{
		glRotated(angle, 0, 0, 1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		glBegin(GL_QUADS);

		//scale /= 2;
		if (drawCentred)
		{
			glTexCoord2i(0,0); glVertex3f(-0.5f/scale*surface->w ,-0.5f/scale*surface->h,0); //bottom left
			glTexCoord2i(1,0); glVertex3f(0.5f/scale*surface->w,-0.5f/scale*surface->h,0); //bottom right
			glTexCoord2i(1,1); glVertex3f(0.5f/scale*surface->w,0.5f/scale*surface->h,0); //top right
			glTexCoord2i(0,1); glVertex3f(-0.5f/scale*surface->w,0.5f/scale*surface->h,0); //top left
		}
		else
		{
			glTexCoord2i(0,0); glVertex3f(0 ,0,0); //bottom left
			glTexCoord2i(1,0); glVertex3f(1.0f/scale*surface->w,0,0); //bottom right
			glTexCoord2i(1,1); glVertex3f(1.0f/scale*surface->w,1.0f/scale*surface->h,0); //top right
			glTexCoord2i(0,1); glVertex3f(0,1.0f/scale*surface->h,0); //top left
		}
	
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}
		glPopMatrix();

}



Font::Font(const char * filename, int newWidth, int newHeight) : Texture(filename), width(newWidth), height(newHeight)
{

}

Font::~Font()
{

}

void Font::DrawText(const char * string, int x, int y, double angle, double scale)
{
	#ifdef DEBUG
		printf("Font::DrawText - drawing \"%s\"\n", string);
	#endif //DEBUG
	glPushMatrix(); //NOT deprecated
	glTranslatef(x, y,0);
	glRotated(angle, 0, 0, 1);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);


	for (int ii=0; string[ii] != '\0'; ++ii)
	{
		if (string[ii] != ' ')
		{
			glPushMatrix();
			glTranslatef(ii*(float)(width)/(float)(scale),0,0);

			int index = (int)(string[ii]) - (int)('!');
			if (index < 0 || index > (int)('~') - (int)('!'))
				index = (int)('~') - (int)('!') + 1;

			float start = (float)(((((float)(index))*((float)(width)))-3.0f)/((float)surface->w));
			float end = (float)(((((float)(index+1))*((float)(width)))-4.0f)/((float)surface->w));
			if (start < 0) {start = 0;} if (end > 1) {end = 1;}
			glBegin(GL_QUADS);
			glTexCoord2f(start,0); glVertex3f(-0.5f/scale*width ,-0.5f/scale*height,0); //bottom left
			glTexCoord2f(end,0); glVertex3f(0.5f/scale*width,-0.5f/scale*height,0); //bottom right
			glTexCoord2f(end,1); glVertex3f(0.5f/scale*width,0.5f/scale*height,0); //top right
			glTexCoord2f(start,1); glVertex3f(-0.5f/scale*width,0.5f/scale*height,0); //top left
			//printf("Index %d - Drawing %c - maps to %f->%f\n", index,string[ii],start,end);
			
			glEnd();
			glPopMatrix();
		}
	}

	
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();

}


void Graphics::Initialise(const char * caption, int newWidth, int newHeight)
{
	if (Initialised())
	{
		std::cerr << "Graphics have already been initialised! Fatal Error\n";
		exit(EXIT_FAILURE);
	}
	screenWidth = newWidth; screenHeight = newHeight;

    	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		std::cerr << "Couldn't init SDL!\n";
	        exit(EXIT_FAILURE);
	}
      //	atexit(Graphics::Destroy); BREAKS THINGS

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1); //According to sulix does not matter. (much)


	

	screen = SDL_SetVideoMode(screenWidth,screenHeight, 32, SDL_OPENGL);
	if ( screen == NULL )
    	{
		std::cerr << "Couldn't set " << screenWidth << "x" << screenHeight << "x32 video mode: " << SDL_GetError() << "\n";
      		exit(EXIT_FAILURE);
	} 

	//COMES AFTER SETVIDEO MODE
	glEnable(GL_TEXTURE_2D);
	glClearColor(1,1,1,0); //Set clear colour (white) here
	glViewport(0,0,screenWidth,screenHeight);	//DOES matter
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,screenWidth,screenHeight,0,-1,1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	SDL_WM_SetCaption( caption, NULL);

	Graphics::initialised = true;
	
}

void Graphics::Destroy()
{
	list<SDL_Surface*>::iterator i(allTextures.begin());
	while (i != allTextures.end())
	{
		SDL_FreeSurface((*i));
		++i;
	}
	SDL_Quit();
}

SDL_Surface * Graphics::LoadTextureBMP(const char * file)
{
		SDL_Surface * tmp = SDL_LoadBMP(file);
		if (tmp == NULL)
			return NULL;
		//assert(tmp != NULL);
	

		if (Graphics::screen != NULL)
		{
			SDL_Surface * tex = SDL_DisplayFormat(tmp);
			SDL_FreeSurface(tmp);
		
			allTextures.push_back(tex);
			return tex;
		}
		return tmp;
}

void Graphics::SaveTextureBMP(SDL_Surface * tex, const char * file)
{
	SDL_SaveBMP(tex, file);
}


void Graphics::DrawTexture(SDL_Surface * tex, int destX, int destY, int srcX, int srcY, int w, int h)
{
	if (w < 0) {w = tex->w - srcX;}
	if (h < 0) {h = tex->h - srcY;}
	Graphics::DrawTexture(screen, tex, destX, destY, srcX, srcY, w, h);
}

void Graphics::DrawTexture(SDL_Surface * dest, SDL_Surface * tex, int destX, int destY, int srcX, int srcY, int width, int height)
{
	if ((destX < 0)||(destX >= dest->w)||(destY < 0)||(destY >= dest->h)
		||(srcX < 0)||(srcX >= tex->w)||(srcY < 0)||(srcY >= tex->h))
		return;

	assert(dest->format->BitsPerPixel == 32);
	assert(tex->format->BitsPerPixel == 32);
	
	if (SDL_MUSTLOCK(tex))   
		SDL_LockSurface(tex);

	if (SDL_MUSTLOCK(dest))   
		SDL_LockSurface(dest);

	
	
	Colour transparent = Graphics::GetPixel(tex, srcX, srcY);
	//printf("transparent from %d %d\n", srcX, srcY);

	for (int xOff = 0; xOff < width; xOff++)
	
	{
		for (int yOff = 0; yOff < height; yOff++)
		{
			Colour nextColour = Graphics::GetPixel(tex, srcX+xOff, srcY+yOff);
			if (nextColour != transparent)
			{
				Graphics::DrawPixel(dest, destX + xOff, destY + yOff, nextColour);
			}
		}
	}	

	if (SDL_MUSTLOCK(tex))
		SDL_UnlockSurface(tex);

	if (SDL_MUSTLOCK(dest))
		SDL_UnlockSurface(dest);
	
}

void Graphics::ClearScreen()
{
	//SDL_FillRect(screen, NULL ,Graphics::MakeColour(0,0,0));
	glClear(GL_COLOR_BUFFER_BIT);

}

void Graphics::UpdateScreen()
{
	SDL_GL_SwapBuffers();
	//SDL_Flip(screen);
}

void Graphics::DrawPixel(int x, int y, Colour colour)
{
	DrawPixel(screen, x, y, colour);
}

void Graphics::DrawPixel(SDL_Surface * dest, int x, int y, Colour colour)
{
	glBegin(GL_POINTS);
	glColor4f(colour.r/255, colour.g/255, colour.b/255, colour.a);
	glVertex2f(x, y);	
	glColor3f(1,1,1);
	glEnd();
}

void Graphics::DrawGrid(int gridWidth, int gridHeight, Colour colour)
{
	for (int x = 0; x < screen->w; x+=gridWidth)
	{
		Graphics::DrawLine(x,0, x,screen->h - 1, colour);
	}
	for (int y = 0; y < screen->h; y+=gridHeight)
	{
		Graphics::DrawLine(0,y, screen->w - 1,y, colour);	
	}
}

Uint8 Graphics::MakeColour(int R, int G, int B, int Alpha)
{
	return SDL_MapRGB(screen->format,R,G,B);
}

Colour Graphics::GetPixel(int x, int y)
{
	return Graphics::GetPixel(screen, x, y);
}

Colour Graphics::GetPixel(SDL_Surface * src, int x, int y)
{ 
	//Convert the pixels to 32 bit 
	Uint8 * pixels = (Uint8*)src->pixels; 
	//Get the requested pixel 

	if (((y > 0)&&(y < src->h)) && ((x > 0)&&(x < src->w)))
		return ConvertColour(pixels[ ( y * src->w ) + x ]); 
	return Colour(0,0,0,0);

} 



void Graphics::DrawLine(int x1, int y1, int x2, int y2, Colour colour,double scale)
{		
	//printf("DRAW LINE\n");
	glColor4f(colour.r/255,colour.g/255,colour.b/255,colour.a);
	glBegin(GL_LINES);
	glVertex2f(x1/scale, y1/scale); // origin of the line
	glVertex2f(x2/scale, y2/scale); // ending point of the line
	glColor3f(1,1,1);
	glEnd();

	
}

void Graphics::DrawLineDashed(int x1, int y1, int x2, int y2, Colour colour, double scale)
{
	glLineStipple(8, 0xAAAA);
	glEnable(GL_LINE_STIPPLE);
	DrawLine(x1,y1,x2,y2,colour,scale);
	glDisable(GL_LINE_STIPPLE);
	glEnd();
}

void Graphics::DrawRectangle(int topX, int topY, int botX, int botY, Colour colour, double scale)
{
	glColor4f(colour.r/255,colour.g/255,colour.b/255,colour.a);
	glBegin(GL_LINES);
	glVertex2f(topX/scale, topY/scale); // origin of the rectangle
	glVertex2f(botX/scale, topY/scale); // point1
	glVertex2f(botX/scale, botY/scale); // point2
	glVertex2f(topX/scale, botY/scale); // point3
	glVertex2f(topX/scale, topY/scale); // point4
	glEnd();
}

Colour Graphics::ConvertColour(Uint8 from)
{
	SDL_PixelFormat * fmt=screen->format;
	Colour result;

	Uint8 temp;
	
	//Get red
	temp=from&fmt->Rmask; /* Isolate red component */
	temp=temp>>fmt->Rshift;/* Shift it down to 8-bit */
	temp=temp<<fmt->Rloss; /* Expand to a full 8-bit number */
	result.r = (float)(temp);

	//Get green
	temp=from&fmt->Gmask; /* Isolate red component */
	temp=temp>>fmt->Gshift;/* Shift it down to 8-bit */
	temp=temp<<fmt->Gloss; /* Expand to a full 8-bit number */
	result.g = (float)(temp);

	//Get blue
	temp=from&fmt->Bmask; /* Isolate red component */
	temp=temp>>fmt->Bshift;/* Shift it down to 8-bit */
	temp=temp<<fmt->Bloss; /* Expand to a full 8-bit number */
	result.b = (float)(temp);

	//Get alpha
	temp=from&fmt->Amask; /* Isolate red component */
	temp=temp>>fmt->Ashift;/* Shift it down to 8-bit */
	temp=temp<<fmt->Aloss; /* Expand to a full 8-bit number */
	result.a = (float)(temp);
	return result;
}

void Graphics::Wait(int n)
{
	SDL_Delay(n);
}

/* Writes an upside down image???
void Graphics::ScreenShot(const char * fileName)
{

	std::vector< GLubyte > pixeldata;

	pixeldata.resize( swidth * sheight * 3 );

	SDL_Surface* image = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth, screenHeight, 24,255U << (16),255 << (8),255 << (0),0);

	SDL_LockSurface( image );

	glReadPixels(0, 0, swidth, sheight, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)image->pixels);



	SDL_UnlockSurface( image );
		
	SDL_SaveBMP(image, fileName);
	SDL_FreeSurface( image );


}
*/


// Hacky code from http://www.gamedev.net/topic/389159-bitmap-file-saving-problem/
// Should probably make it nicer

void Graphics::ScreenShot(const char * fileName)
{

	unsigned char *pixels = (unsigned char*)(malloc (screenWidth * screenHeight * 3));
		
	SDL_Surface* reversed_image = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth, screenHeight, 24,
		255U << (0), // Blue channel
		255 << (8), // Green channel
		255 << (16), // Red channel
		0 /* no alpha! */);

	SDL_LockSurface( reversed_image );

	// Read in the pixel data
	glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	SDL_UnlockSurface( reversed_image );

	/* At this point the image has been reversed, so we need to re-reverse it so that
	it is the correct way around. We do this by copying the "image" pixels to another
	surface in reverse order */
	SDL_Surface* image = SDL_CreateRGBSurface(SDL_SWSURFACE, screenWidth, screenHeight, 24,
		255U << (0), // Blue channel
		255 << (8), // Green channel
		255 << (16), // Red channel
		0 /* no alpha! */);

	uint8_t *imagepixels = reinterpret_cast<uint8_t*>(image->pixels);
	// Copy the "reversed_image" memory to the "image" memory
	for (int y = (screenHeight - 1); y >= 0; --y) {
		uint8_t *row_begin = pixels + y * screenWidth * 3;
		uint8_t *row_end = row_begin + screenWidth * 3;

		std::copy(row_begin, row_end, imagepixels);

		// Advance a row in the output surface.
		imagepixels += image->pitch;
	}
		
	// Save file
	SDL_SaveBMP(image, fileName);
		
	// Clear memory
	SDL_FreeSurface( reversed_image );
	SDL_FreeSurface( image );
}

#endif //BUILD_GRAPHICS
