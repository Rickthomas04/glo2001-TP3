//
// NOM PRENOM :Ntore Darrell Tugizimana & Alexis Roberge
// IDUL :dntug & alrob163
//
#include "FileSystem.h"
#include <cmath>
#include <iomanip>
#include <algorithm>

FileSystem::FileSystem(BlockDevice &d) : device(d) {
    // Démarrage : tous les blocs sont disponibles
    freeBitmap = std::vector<bool>(64, true);
}

void FileSystem::Compact() 
{
    std::cout << "=== Début de la compaction du disque ===" << std::endl;
 
    std::vector<bool> newBitmap(64, true);  //nouveau bitmap pour compacter
    size_t nextFreeBlock = 0; //indice du prochain bloc libre
    
 //Parcourir tous les fichiers 
    for (auto it = root.begin(); it != root.end(); ++it) 
    {
        Inode &inode = it->second;
        std::vector<size_t> newBlockList;
        size_t nbBlocks = inode.blockList.size();
        char buffer[1024]; // buffer temporaire pour le transfert de blocs
        
 //Deplace chaque bloc occupe vers un bloc contigu libre
        for (auto iter = inode.blockList.begin(); iter != inode.blockList.end(); ++iter) 
        {
            device.ReadBlock(*iter, buffer); //lecture de l ancien bloc
            device.WriteBlock(nextFreeBlock, buffer); // Ecriture au nouvel emplacement
            newBitmap[nextFreeBlock] = false; // marque le bloc comme occupe
            newBlockList.push_back(nextFreeBlock); // Sauvegarde l<indice du nouveau bloc
            nextFreeBlock++;
        }
 
        inode.blockList = newBlockList; //Mise a jour de la liste des blocs du fichier
    }
 
    freeBitmap = newBitmap; //Remplace le bitmap par le nouveau
    std::cout << "=== Fin de la compaction. nextFreeBlock = " << nextFreeBlock << " ===" << std::endl;
}

std::vector<size_t> FileSystem::AllocateBlocks(size_t nbBlocs) 
{
    std::vector<size_t> allocatedBlocks;
    //Parcourir le bitmap pour trouver des blocs libres
    for (auto it = freeBitmap.begin(); it != freeBitmap.end() && allocatedBlocks.size() < nbBlocs; ++it) 
    {
        if (*it) 
        {
            size_t index = std::distance(freeBitmap.begin(), it);
            allocatedBlocks.push_back(index); // enregistrement de l indice du bloc
            *it = false; //Marque comme occupe
        }
    }
    return allocatedBlocks;
}

void FileSystem::FreeBlocks(const std::vector<size_t> &blocks) 
{
	// Libere chaque bloc de la liste 
    for (auto it = blocks.begin(); it != blocks.end(); ++it) 
    {
        if (*it < freeBitmap.size()) 
        {
            freeBitmap[*it] = true;
        }
    }
}

bool FileSystem::Create(const std::string &filename, size_t sizeInBytes) 
{
    // vérifie si le fichier existe déjà
    if (root.find(filename) != root.end()) 
    {
        return false;  // fichier existe déjà
    }
 
    // calcule le nombre de blocs nécessaires
    size_t nbBlocs = (sizeInBytes + 1023) / 1024;
 
    // alloue les blocs
    std::vector<size_t> blocks = AllocateBlocks(nbBlocs);
 
    // vérifier que l'allocation a réussi
    if (blocks.size() < nbBlocs) 
    {
        FreeBlocks(blocks);
        return false;
    }
 
    // crée l'inode correspondant
    Inode newInode;
    newInode.fileName = filename;
    newInode.fileSize = sizeInBytes;
 
    // copie des blocs alloués dans le blockList
    for (auto it = blocks.begin(); it != blocks.end(); ++it) 
    {
        newInode.blockList.push_back(*it);
    }
 
    // ajoute l'inode au répertoire racine
    root[filename] = newInode;
 
    std::cout << "Fichier " << filename << " créé, taille " << sizeInBytes
		<< " octets, blocs alloués = " << blocks.size() << std::endl;
    return true;
}


bool FileSystem::Write(const std::string &filename, size_t offset, const std::string &data) 
{   
    //Recherche du fichier dans le repertoire racine
    auto it = root.find(filename);
    Inode &inode = it->second;
    
    //Calcul du bloc où commence l'écriture et de l'offset à l'intérieur de ce bloc
    size_t startBlock = offset / 1024;
    size_t startOffset = offset % 1024;
    size_t remainingBytes = data.length(); //Nombre total d'octets à écrire
    size_t dataOffset = 0;  //position courante dans la chaine de données à écrire
 
    auto iter = inode.blockList.begin() + startBlock; //Position de depart dans la liste des blocs du fichier
    
    //Boucle while tant qu'il reste des octets à écrire et des blocs disponibles
    while (remainingBytes > 0 && iter != inode.blockList.end()) 
    {
        char buffer[1024]; //Buffer temporaire pour le bloc courant
        device.ReadBlock(*iter, buffer); //lecture du contenu actuel
        
        //Calcul du nombre d'octets que l'on peut écrire dans ce bloc 
        size_t bytesToWrite = std::min(remainingBytes, 1024 - startOffset);
        
        for (size_t i = 0; i < bytesToWrite; i++) //Copie des octets dans le buffer du bon offset
        {
            buffer[startOffset + i] = data[dataOffset + i];
        }
 
        device.WriteBlock(*iter, buffer); //Ecriture du buffer modifie dans le bloc du disque
        
        //Mis a jour des compteurs
        remainingBytes -= bytesToWrite;
        dataOffset += bytesToWrite;
        ++iter;
        startOffset = 0;
    }
 
    std::cout << "Write dans " << filename << " (" << data.length() << " octets)" << std::endl;
    return true;
}

bool FileSystem::Read(const std::string &filename, size_t offset, size_t length, std::string &outData) 
{
    //Recherche du fichier dans le repertoire racine
    auto it = root.find(filename);
    Inode &inode = it->second;
    
    //Preparation de la chaine de sortie
    outData.clear();
    outData.reserve(length);
    
    //Calcul du bloc où commence l'écriture et de l'offset à l'intérieur de ce bloc
    size_t startBlock = offset / 1024;
    size_t startOffset = offset % 1024;
    
    size_t remainingBytes = length; // Nombre total d'octets à lire
 
    auto iter = inode.blockList.begin() + startBlock; // Point de départ dans la liste des blocs du fichier
 
    // Boucle tant qu'il reste des données à lire et des blocs disponibles
    while (remainingBytes > 0 && iter != inode.blockList.end()) 
    {
        char buffer[1024]; // Buffer temporaire pour stocker le contenu du bloc
        device.ReadBlock(*iter, buffer); // Lecture du bloc en mémoire
        
        // Calcul du nombre d'octets on peut lire à partir de ce bloc
        size_t bytesToRead = std::min(remainingBytes, 1024 - startOffset);
        
        outData.append(buffer + startOffset, bytesToRead); // Ajout des données lues dans la chaîne de sortie
        
        // Mise à jour des compteurs pour passer au bloc suivant
        remainingBytes -= bytesToRead;
        ++iter;
        startOffset = 0;
    }
 
    std::cout << "Read dans " << filename << " (" << length << " octets)" << std::endl;
    return true;
}


bool FileSystem::Delete(const std::string &filename) 
{
    auto it = root.find(filename);
    FreeBlocks(it->second.blockList);  // Libère l’espace utilisé
    root.erase(it);  // Retire l’inode du répertoire
    std::cout << "Fichier " << filename << " supprimé" << std::endl;
    return true;
}


void FileSystem::List() 
{
    std::cout << "Liste des fichiers :" << std::endl;
    
    // Parcourt chaque fichier enregistré dans le répertoire racine
    for (auto it = root.begin(); it != root.end(); ++it) 
    {
        const Inode &inode = it->second;  // Récupère l'inode correspondant au fichier
        std::cout << "- " << inode.fileName << " : size " << inode.fileSize
<< ", nbBlocs=" << inode.blockList.size() << std::endl;
    }
}

