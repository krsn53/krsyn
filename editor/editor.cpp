#include <QApplication>
#include "KrsynEditorMain.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KrsynEditorMain* window = new KrsynEditorMain();
    window->show();

    return app.exec();
}
