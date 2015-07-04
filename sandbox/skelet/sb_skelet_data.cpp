#include "sb_skelet_data.h"
#include "sb_resources.h"
#include <ghl_data_stream.h>
#include <ghl_data.h>
#include <pugixml.hpp>
#include "sb_log.h"
#include "sb_base64.h"
#include "sb_data.h"
#include "sb_image.h"

SB_META_DECLARE_OBJECT(Sandbox::SkeletonAnimation,Sandbox::meta::object)
SB_META_DECLARE_OBJECT(Sandbox::SkeletonData,Sandbox::meta::object)

namespace Sandbox {
    
    SkeletonAnimation::SkeletonAnimation() : m_data(0),m_fps(15.0),m_nodes(0),m_frames(0) {}
    
    SkeletonAnimation::~SkeletonAnimation() {
        delete [] m_data;
    }
    
    SkeletonNodeFrame* SkeletonAnimation::AllocData(size_t nodes,size_t frames) {
        delete [] m_data;
        m_nodes = nodes;
        m_frames = frames;
        m_data = new SkeletonNodeFrame[m_frames*m_nodes];
        return m_data;
    }
    
    const SkeletonNodeFrame& SkeletonAnimation::GetNodeFrame(size_t frame,size_t node) const {
        sb_assert(frame<m_frames);
        sb_assert(node<m_nodes);
        return m_data[frame*m_nodes+node];
    }
    
    void SkeletonData::AddAnimation(const sb::string& name, const SkeletonAnimationPtr& animation) {
        sb_assert(animation);
        sb_assert(animation->GetNodesCount()==m_nodes_count);
        m_animations[name] = animation;
    }
    
    union data_read_u {
        float       fvalue;
        GHL::UInt32 uvalue;
        GHL::Byte   data[4];
    };
        
    SkeletonDataPtr SkeletonData::Load(const char* filename, Resources* resources) {
        GHL::DataStream* ds = resources->OpenFile(filename);
        if (!ds) return SkeletonDataPtr();
        GHL::Data* data = GHL_ReadAllData(ds);
        ds->Release();
        if (!data)
            return SkeletonDataPtr();
        
        sb::string dir;
        const char* endslash = ::strrchr(filename, '/');
        if (endslash) {
            dir.assign(filename,endslash+1);
        }
        
        pugi::xml_document doc;
        pugi::xml_parse_result parse_res = doc.load_buffer(data->GetData(),
                                                           data->GetSize(),
                                                           pugi::parse_default, pugi::encoding_utf8);
        data->Release();
        if (!parse_res) {
            LogError() << "failed parse " << filename;
            LogError() << parse_res.description();
            return SkeletonDataPtr();
        }
        pugi::xml_node n = doc.document_element();
        pugi::xml_node textures = n.child("textures");
        pugi::xml_node nodes = n.child("nodes");
        if (!nodes)
            return SkeletonDataPtr();
        pugi::xml_node animations = n.child("animations");
        if (!animations)
            return SkeletonDataPtr();
        size_t nodes_count = nodes.attribute("count").as_uint();
        SkeletonDataPtr res(new SkeletonData());
        if (textures) {
            for (pugi::xml_node_iterator it = textures.begin(); it!=textures.end(); ++it) {
                const char* file = it->attribute("file").value();
                TexturePtr texture = resources->GetTexture((dir+file).c_str(), !it->attribute("premultiplied").as_bool());
                if (!texture) {
                    LogError() << "not found texture " << file;
                    continue;
                }
                texture->SetFiltered(true);
                for (pugi::xml_node_iterator iit = it->begin();iit!=it->end();++iit) {
                    ImagePtr img = ImagePtr(new Image(texture,iit->attribute("x").as_float(),
                                             iit->attribute("y").as_float(),
                                             iit->attribute("w").as_float(),
                                             iit->attribute("h").as_float()));
                    bool r = iit->attribute("r").as_bool();
                    img->SetRotated(r);
                    if (r) {
                        img->SetSize(iit->attribute("h").as_float(),iit->attribute("w").as_float());
                    }
                    res->SetImage(iit->attribute("index").as_uint(),iit->attribute("name").value(),img);
                    img->SetHotspot(Sandbox::Vector2f(iit->attribute("hsx").as_float(),iit->attribute("hsy").as_float()));
                }
            }
        }
        res->SetNodesCount(nodes_count);
        for (pugi::xml_node_iterator it = nodes.begin();it!=nodes.end();++it) {
            size_t index = it->attribute("index").as_uint();
            sb_assert(index < nodes_count);
            res->AddNodeMap(it->attribute("name").value(), index);
        }
        for (pugi::xml_node_iterator it = animations.begin();it!=animations.end();++it) {
            SkeletonAnimationPtr anim(new SkeletonAnimation());
            anim->SetFPS(it->attribute("fps").as_float());
            size_t frames = it->attribute("frames").as_uint();
            SkeletonNodeFrame* pdata = anim->AllocData(nodes_count, frames);
            size_t data_size = (sizeof(float)*(4+2)+sizeof(float)+sizeof(GHL::UInt32))*frames*nodes_count;
            GHL::Data* d = 0;
            if (::strcmp(it->attribute("encoding").as_string(),"base64")==0) {
                d = Base64DecodeData(it->child_value());
            } else {
                LogError() << "unsupported encoding " << it->attribute("encoding").as_string();
                continue;
            }
            
            if (!d) continue;
            if (::strcmp(it->attribute("compression").as_string(),"zlib")==0) {
                GHL::UInt32 size = data_size;
                VectorData<GHL::Byte>* dd = new VectorData<GHL::Byte>();
                dd->vector().reserve(size);
                dd->vector().resize(size);
                
                if( GHL_UnpackZlib(d, dd->vector().data(), &size ) && size == dd->GetSize() ) {
                    d->Release();
                    d = dd;
                } else {
                    dd->Release();
                    d->Release();
                    continue;
                }
            } else if (it->attribute("compression")) {
                LogError() << "unsupported compression " << it->attribute("compression").as_string();
                d->Release();
                continue;
            }
            const float* src = reinterpret_cast<const float*>(d->GetData());
            for (size_t i=0;i<frames;++i) {
                for (size_t n=0;n<nodes_count;++n) {
                    pdata->alpha = *src++;
                    pdata->transform.m.matrix[0] = *src++;
                    pdata->transform.m.matrix[1] = *src++;
                    pdata->transform.m.matrix[2] = *src++;
                    pdata->transform.m.matrix[3] = *src++;
                    pdata->transform.v.x = *src++;
                    pdata->transform.v.y = *src++;
                    pdata->image = *reinterpret_cast<const GHL::UInt32*>(src);
                    src++;
                    ++pdata;
                }
            }
            res->AddAnimation(it->attribute("name").value(), anim);
        }
        return res;
    }
    
    
    void SkeletonData::AddNodeMap(const sb::string& name, size_t node_indx ) {
        sb_assert(node_indx<m_nodes_count);
        m_nodes_map[name]=node_indx;
    }
    static const SkeletonAnimationPtr empty;
    const SkeletonAnimationPtr& SkeletonData::GetAnimation(const sb::string& name) const {
        sb::map<sb::string,SkeletonAnimationPtr>::const_iterator it = m_animations.find(name);
        if (it == m_animations.end())
            return empty;
        return it->second;
    }
    static const ImagePtr empty_image;
    const ImagePtr& SkeletonData::GetImage(size_t index) const {
        if (index >= m_images.size()) {
            return empty_image;
        }
        return m_images[index];
    }
    
    void SkeletonData::SetImage(size_t index,const sb::string& name, const ImagePtr& img) {
        if (index >= m_images.size()) {
            m_images.resize(index+1);
        }
        m_images[index] = img;
        m_images_map[name]=index;
    }
    
}