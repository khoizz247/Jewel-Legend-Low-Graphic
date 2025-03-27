#include "texturemanager.h"

TextureManager* TextureManager::s_pInstance = nullptr; //Initialize, singleton

TextureManager* TextureManager::Instance() {
    if(s_pInstance == nullptr) {
        s_pInstance = new TextureManager();
    }
    return s_pInstance;
}

TextureManager::TextureManager() {} //Private, singleton
TextureManager::~TextureManager() {
    clear(); // Giải phóng tất cả textures khi TextureManager bị hủy
}

SDL_Texture* TextureManager::loadTexture(const std::string& filePath, SDL_Renderer* renderer) {
    // Check if the texture is already loaded
    if (m_textureMap.find(filePath) != m_textureMap.end()) {
        return m_textureMap[filePath]; // Texture đã được load rồi, lấy từ cache
    }

    SDL_Texture* texture = IMG_LoadTexture(renderer, filePath.c_str());
    if (texture == nullptr) {
        std::cerr << "Failed to load texture: " << filePath << " Error: " << IMG_GetError() << std::endl;
        return nullptr;
    }

    m_textureMap[filePath] = texture; // Cache texture vừa load
    return texture;
}

void TextureManager::clear() {
    // Destroy all textures and clear the cache
    for (auto const& [key, val] : m_textureMap) {
        if(val != nullptr){
            SDL_DestroyTexture(val);
        }
    }
    m_textureMap.clear();
}
