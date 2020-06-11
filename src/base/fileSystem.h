#pragma once

/// \file base/fileSystem.h
///
/// File system utilities.

inline std::string GetParentDirectory( const std::string& i_path )
{
    return i_path.substr( 0, i_path.find_last_of( '/' ) );
}

struct BothSlashes
{
    bool operator()( char a, char b ) const
    {
        return a == '/' && b == '/';
    }
};

inline std::string SanitizePath( const std::string& i_path )
{
    std::string path = i_path;
    path.erase( std::unique( path.begin(), path.end(), BothSlashes() ), path.end() );
    return path;
}

inline std::string JoinPaths( const std::string& i_pathA, const std::string& i_pathB )
{
    std::string path = i_pathA + "/" + i_pathB;
    return SanitizePath( path );
}

/// Read a file, and return the array of binary data.
inline std::vector< char > ReadFile( const std::string& i_filePath )
{
    std::ifstream file( i_filePath, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
        throw std::runtime_error( "failed to open file!" );
    }

    // Read the entire file.
    size_t              fileSize = ( size_t ) file.tellg();
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );
    file.read( buffer.data(), fileSize );
    file.close();

    return buffer;
}

