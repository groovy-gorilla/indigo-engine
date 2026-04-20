#include "ErrorDialog.h"
#include <memory>

#include "System.h"
#include "Global.h"

Settings SETTINGS;

int main() {

    // Inteligentny wskaźnik
    auto system = std::make_unique<System>();


    try {
        system->Initialize();
        system->Run();
    } catch (const std::exception &e) {
        ShowErrorDialog(EscapeMarkup(e.what()));
    }

    return EXIT_SUCCESS;

}