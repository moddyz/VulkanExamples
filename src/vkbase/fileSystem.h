#pragma once

/// \file vkbase/fileSystem.h
///
/// Common file system utilities.

namespace vkbase
{
/// Functor for backslash '/' detection.
struct BothSlashes
{
    bool operator()( char a, char b ) const
    {
        return a == '/' && b == '/';
    }
};

/// Sanitize the path \p i_path by reducing multiple consecutive forward slashes ("/")
/// into a single one.
inline std::string SanitizePath( const std::string& i_path )
{
    std::string path = i_path;
    path.erase( std::unique( path.begin(), path.end(), BothSlashes() ), path.end() );
    return path;
}

/// Get the path identified as the parent of \p i_path.
///
/// \param i_path the path to extract its parent from.
///
/// \return the parent path.
inline std::string GetParentPath( const std::string& i_path )
{
    std::string sanitizedPath = SanitizePath( i_path );
    return sanitizedPath.substr( 0, i_path.find_last_of( '/' ) );
}

/// Join two paths \p i_lhs and \p i_rhs by the "/" delimiter.
///
/// \param i_lhs the left hand side path.
/// \param i_rhs the right hand side path.
///
/// \return the joined path.
inline std::string JoinPaths( const std::string& i_lhs, const std::string& i_rhs )
{
    std::string path = i_lhs + "/" + i_rhs;
    return SanitizePath( path );
}

/// Read the file at \p i_filePath, and return an array of binary data.
///
/// \param i_filePath the path to the file to read.
///
/// \return the binary data in the form of a vector of bytes.
inline std::vector< char > ReadFile( const std::string& i_filePath )
{
    std::ifstream file( i_filePath, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
        throw std::runtime_error( "Failed to open file." );
    }

    // Read the entire file.
    size_t              fileSize = ( size_t ) file.tellg();
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );
    file.read( buffer.data(), fileSize );
    file.close();

    return buffer;
}

} // namespace vkbase
