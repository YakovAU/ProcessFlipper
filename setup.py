# setup.py
from setuptools import setup
from Cython.Build import cythonize

setup(
    name='ProcessKillerSuspender',
    version='1.0',
    ext_modules=cythonize("process_killer.pyx"),
    install_requires=[
        'pynput',
        'pywin32',
    ],
    py_modules=['window_handler', 'main'],
    entry_points={
        'console_scripts': [
            'processkillersuspender=main:main',
        ],
    },
)
