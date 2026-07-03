qtHaveModule(httpserver) {
    QT += httpserver
} else {
    error("Qt HttpServer (qthttpserver) is required. Qt Maintenance Tool -> Qt 6.x -> Qt HttpServer -> Apply, then Run qmake.")
}
