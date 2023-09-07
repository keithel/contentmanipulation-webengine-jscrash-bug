# Resources:
set(resource_files
    "jquery.min.js"
    "page.html"
)

qt_add_resources(contentmanipulation "resources"
    PREFIX
        "/"
    FILES
        ${resource_files}
)
