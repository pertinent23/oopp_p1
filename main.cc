#include "belote.hh"
#include <iostream>

int main() {
    // On appelle ta fonction en branchant l'entrée standard (clavier/fichier), 
    // la sortie standard (console) et la sortie d'erreur
    bool result = game(std::cin, std::cout, std::cerr);
    
    if (!result) {
        std::cerr << "Le jeu s'est arrêté à cause d'une erreur (triche ou format).\n";
        return 1;
    }
    
    return 0;
}