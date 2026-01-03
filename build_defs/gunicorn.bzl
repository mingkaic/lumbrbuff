load("@rules_python//python:defs.bzl", "py_binary", "py_library")

# from https://gist.github.com/ewhauser/3c6eaaec507e96e068cc6633a3ec135a
def gunicorn_binary(name, srcs, main = None, deps = [], args = [], **kwargs):
    py_library(
        name = name + "_lib",
        srcs = srcs,
        deps = deps,
    )

    py_binary(
        name = name,
        srcs = ["//build_defs/gunicorn:__main__.py"],
        main = "//build_defs/gunicorn:__main__.py",
        deps = [
            "@pip//gunicorn:pkg",
            ":" + name + "_lib",
        ],
        args = args,
        **kwargs
    )

    if main != None:
        py_binary(
            name = name + "_dev",
            srcs = srcs,
            main = main,
            deps = deps,
            **kwargs
        )
