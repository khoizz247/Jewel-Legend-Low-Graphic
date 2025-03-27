#ifndef TEXTUREMANAGER_H
#define TEXTUREMANAGER_H

#include <SDL.h>
#include <SDL_image.h> // Cần thiết cho IMG_LoadTexture

#include <iostream>
#include <map>
#include <string>

class TextureManager {
public:
    static TextureManager* Instance(); //Singleton
    SDL_Texture* loadTexture(const std::string& filePath, SDL_Renderer* renderer);
    void clear();

private:
    TextureManager(); //Private, singleton only
    ~TextureManager();

    static TextureManager* s_pInstance;
    std::map<std::string, SDL_Texture*> m_textureMap; // Cache loaded textures.
};

#endif
