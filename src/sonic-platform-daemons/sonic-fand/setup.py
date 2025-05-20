from setuptools import setup

setup(
    name='sonic-fand',
    version='1.0',
    description='FAND daemon for SONiC',
    license='Apache 2.0',
    author='SONiC Team',
    author_email='wangzhui@asterfusion.com',
    url='https://github.com/Azure/sonic-platform-daemons',
    maintainer='Kevin Wang',
    maintainer_email='kevinw@mellanox.com',
    scripts=[
        'scripts/fand',
    ],
    classifiers=[
        'Development Status :: 4 - Beta',
        'Environment :: No Input/Output (Daemon)',
        'Intended Audience :: Developers',
        'Intended Audience :: Information Technology',
        'Intended Audience :: System Administrators',
        'License :: OSI Approved :: Apache Software License',
        'Natural Language :: English',
        'Operating System :: POSIX :: Linux',
        'Programming Language :: Python :: 3.7',
        'Topic :: System :: Hardware',
    ],
    keywords='sonic SONiC fan FAN daemon fand FAND',
)
