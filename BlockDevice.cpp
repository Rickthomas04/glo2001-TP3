//
// NOM PRENOM :Ntore Darrell Tugizimana & Alexis Roberge
// IDUL :dntug & alrob163
//
#include "BlockDevice.h"

// Constructeur : initialise un disque virtuel de 64 Ko, soit 64 blocs de 1024 octets
BlockDevice::BlockDevice() {
    disk = std::vector<char>(64 * 1024, 0); // Remplit le disque de zéros
}

bool BlockDevice::ReadBlock(size_t i, char* buff) {
    // Vérification que l’indice du bloc est < 64
    if (i >= 64) return false;

    size_t pos = i * 1024; // Calcul de la position de départ du bloc dans le vecteur
    
    //Copie les 1024 octets du bloc dans le buffer
    for (size_t j = 0; j < 1024; ++j)
        buff[j] = disk[pos + j];

    return true;
}

bool BlockDevice::WriteBlock(size_t i, const char* data) {
    // Vérifie que l’indice du bloc est < 64 
    if (i >= 64) return false;

    size_t pos = i * 1024;  //Copie les 1024 octets du tableau dans le bloc du disque
    for (size_t j = 0; j < 1024; ++j)
        disk[pos + j] = data[j];

    return true;
}
