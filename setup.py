from __future__ import print_function

from distutils.command.build_py import build_py
import os
import shutil
import setuptools
import subprocess
import sys


LIBRARY_TYPE = 'dylib' if sys.platform == 'darwin' else 'so'
BUILT_LIB_FILENAME = 'libpghstorers.{}'.format(LIBRARY_TYPE)
PACKAGE = 'pghstore'


def build_rustlib(base_path):
    """Build our cargo library."""
    library_path = os.path.join(base_path, BUILT_LIB_FILENAME)
    current_working_dir = os.path.abspath(os.path.dirname(__file__))
    cargo_build = ['cargo', 'build', '--release']
    if sys.stdout.isatty():
        cargo_build.append('--color=always')

    exit_code = subprocess.Popen(cargo_build, cwd=current_working_dir).wait()
    if exit_code != 0:
        sys.exit(exit_code)

    built_lib = os.path.join(
        current_working_dir, 'target', 'release', BUILT_LIB_FILENAME,
    )
    if os.path.isfile(built_lib):
        print('copying {} to {}'.format(built_lib, library_path))
        shutil.copy2(built_lib, library_path)


class BuildPy(build_py):
    """Replace our default build command."""

    def run(self):
        """Run our default build process and include our Rust library."""
        build_py.run(self)
        build_rustlib(os.path.join(self.build_lib, PACKAGE))


c_speedups = setuptools.Feature(
    'optional C speed-enhancement module',
    standard=True,
    available=not ('java' in sys.platform or
                   getattr(sys, 'pypy_version_info', None)),
    ext_modules=[
        setuptools.Extension(
            'pghstore._speedups', ['src/pghstore/_speedups.c'],
            extra_compile_args=['-O3']
        ),
    ]
)


setuptools.setup(
    name='pghstore',
    packages=[PACKAGE],
    package_dir={'': 'src'},
    version='3.0.0',
    features={
        'cspeedups': c_speedups,
    },
    install_requires=['six', 'cffi>=1.6.0'],
    cmdclass={
        'build_py': BuildPy,
    },
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Operating System :: OS Independent',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: Implementation :: CPython',
        'Programming Language :: Python :: Implementation :: PyPy',
        'Topic :: Database',
        'Topic :: Software Development :: Libraries :: Python Modules'
    ],
)
