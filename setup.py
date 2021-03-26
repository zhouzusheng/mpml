from setuptools import setup, Extension
from Cython.Build import cythonize


module_ahcp = Extension(
    "ahcp", ["src/ahcp.pyx", "src/ahcp/aho_corasick.c", "src/ahcp/ac_list.c"],
    #extra_compile_args=["-DDEBUG","/Z7","/Od"],
    #extra_link_args=["/DEBUG"]
)

module_sregex = Extension(
    "sregex", ["src/sregex.pyx", "src/sregex/sregex_wrap.c",
              "src/sregex/sre_palloc.c",
              "src/sregex/sre_regex.c",
              "src/sregex/sre_yyparser.c",
              "src/sregex/sre_regex_compiler.c",
              "src/sregex/sre_vm_pike.c",
              "src/sregex/sre_capture.c",
              ],
    extra_compile_args=["-DSREGEX_STATIC"],
    #extra_link_args=["/DEBUG"]
)

with open("README.md", "r") as readme:
    long_description = readme.read()

setup(
    name="mpml",
    version="1.0.1",
    description="multi pattern match library",
    long_description=long_description,
    long_description_content_type="text/markdown",
    classifiers=[
        "Development Status :: Beta",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: "
        "GNU Library or Lesser General Public License (LGPL)",
        "Operating System :: POSIX",
        "Programming Language :: C",
        "Programming Language :: Python",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Text Processing :: Indexing",
    ],
    install_requires=["setuptools","Cython"],
    author="zhouzusheng",
    author_email="zhouzusheng@aitek.ai",
    license="GNU LGPL",
    platforms=["POSIX"],
    ext_modules=cythonize([module_ahcp,module_sregex]),
    package_dir={"": "src"},
    py_modules=["mpml"],
)
