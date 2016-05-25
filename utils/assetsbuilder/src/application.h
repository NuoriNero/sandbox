#ifndef APPLICATION_H_INCLUDED
#define APPLICATION_H_INCLUDED

#include <sbstd/sb_string.h>
#include <sbstd/sb_vector.h>
#include <sb_lua.h>
#include <sb_file_provider.h>
#include <ghl_vfs.h>
#include <meta/sb_meta.h>

namespace GHL {
    struct ImageDecoder;
    struct Image;
}

class SkeletonConvert;
class SpineConvert;

class Texture : public Sandbox::meta::object {
    SB_META_OBJECT
private:
    GHL::UInt32         m_width;
    GHL::UInt32         m_height;
public:
    explicit Texture( GHL::UInt32 w, GHL::UInt32 h) :
        m_width(w), m_height(h) {}
    GHL::UInt32 width() const { return m_width; }
    GHL::UInt32 height() const { return m_height; }
};
typedef sb::intrusive_ptr<Texture> TexturePtr;

class TextureData : public Texture {
    SB_META_OBJECT
private:
    GHL::Image* m_data;
public:
    explicit TextureData( GHL::UInt32 w, GHL::UInt32 h );
    explicit TextureData( GHL::Image* img );
    ~TextureData();
    void PremultiplyAlpha();
    const GHL::Image* GetImage() const { return m_data; }
    void Place( GHL::UInt32 x, GHL::UInt32 y, const sb::intrusive_ptr<TextureData>& img );
    bool SetAlpha( const sb::intrusive_ptr<TextureData>& alpha_tex );
};
typedef sb::intrusive_ptr<TextureData> TextureDataPtr;

class Application : public Sandbox::FileProvider {
    SB_META_OBJECT
public:
	Application();
	virtual ~Application();

    void set_update_only(bool u);
	void set_paths(const sb::string& scripts, const sb::string& src, const sb::string& dst);
	void set_platform(const sb::string& platform);
    void set_arguments(const sb::vector<sb::string>& arguments);

    TexturePtr check_texture( const sb::string& file );
    TextureDataPtr load_texture( const sb::string& file );
    bool store_texture( const sb::string& file , const TextureDataPtr& data );
    bool store_file(  const sb::string& file , const GHL::Data* data );
    bool write_text_file( const sb::string& file , const char* data  );
    sb::intrusive_ptr<SpineConvert> open_spine(const sb::string& atlas,
                                                const sb::string& skelet );
    bool convert_spine(const sb::string& atlas,
                       const sb::string& skelet,
                       const sb::string& outfolder);
    bool premultiply_image( const sb::string& src, const sb::string& dst );
    
	int run();
protected:
    virtual double GetVersion() const;
    /// FileProvider
    virtual GHL::DataStream* OpenFile(const char* fn);
    Sandbox::LuaVM* GetLua() { return m_lua; }
    virtual void Bind(lua_State* L);
    sb::string get_output_filename( const char* name );
private:
	Sandbox::LuaVM*	m_lua;
    GHL::VFS*   m_vfs;
    GHL::ImageDecoder* m_image_decoder;
    sb::string  m_scripts_dir;
    sb::string  m_dst_dir;
    sb::string  m_src_dir;
    sb::string  m_platform;
    bool        m_update_only;
    sb::vector<sb::string> m_arguments;
};

#endif /*APPLICATION_H_INCLUDED*/
