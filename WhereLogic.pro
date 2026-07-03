TEMPLATE = subdirs

SUBDIRS = \
    game \
    presenter \
    setup \
    tests

!contains(CONFIG, no_opencv) {
    SUBDIRS = opencv_external $$SUBDIRS
    opencv_external.file = external/WhereLogicOpenCV/WhereLogicOpenCV.pro
    game.depends = opencv_external
}

game.file = game/WhereLogicGame.pro
presenter.file = presenter/WhereLogicPresenter.pro
setup.file = tools/WhereLogicSetup/WhereLogicSetup.pro
tests.file = tests/tests.pro
