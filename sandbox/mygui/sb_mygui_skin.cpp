#include "sb_mygui_skin.h"
#include "MyGUI_FactoryManager.h"
#include "MyGUI_SubWidgetManager.h"
#include "meta/sb_meta.h"
#include "MyGUI_RenderItem.h"
#include "MyGUI_ITexture.h"
#include "widgets/sb_mygui_mask_image.h"
#include "widgets/sb_mygui_mask_text.h"
#include "widgets/sb_mygui_scene_object.h"
#include "sb_mygui_text_skin.h"
#include "sb_mygui_render.h"
#include "sb_graphics.h"

SB_META_DECLARE_OBJECT(Sandbox::mygui::ColorizedSubSkinStateInfo,MyGUI::SubSkinStateInfo)

SB_META_DECLARE_OBJECT(Sandbox::mygui::ColorizedSubSkin,MyGUI::SubSkin)
SB_META_DECLARE_OBJECT(Sandbox::mygui::CopySubSkin,MyGUI::SubSkin)

SB_META_DECLARE_OBJECT(Sandbox::mygui::MaskSubSkin,MyGUI::SubSkin)
SB_META_DECLARE_OBJECT(Sandbox::mygui::MaskSetSubSkin,MyGUI::MainSkin)
SB_META_DECLARE_OBJECT(Sandbox::mygui::MaskSetSubSkinState,MyGUI::SubSkinStateInfo)

namespace Sandbox {

    namespace mygui {
        
     
        void ColorizedSubSkin::applyColor() {
            MyGUI::Colour val(m_color.red*m_state_color.red,
                              m_color.green*m_state_color.green,
                              m_color.blue*m_state_color.blue,
                              m_color.alpha*m_state_color.alpha);
            MyGUI::uint32 colour = MyGUI::texture_utility::toColourARGB(val);
            mCurrentColour = (colour & 0x00FFFFFF) | (mCurrentColour & 0xFF000000);
        }
        void ColorizedSubSkin::_setColour(const MyGUI::Colour& _value) {
            m_color = _value;
            applyColor();
            if (nullptr != mNode)
                mNode->outOfDate(mRenderItem);
        }
        void ColorizedSubSkin::setStateData(MyGUI::IStateInfo* _data) {
            MyGUI::SubSkin::setStateData(_data);
            m_state_color = _data->castType<ColorizedSubSkinStateInfo>()->getColor();
            
            applyColor();
            
            if (nullptr != mNode)
                mNode->outOfDate(mRenderItem);
        }
        
        void CopySubSkin::_setUVSet(const MyGUI::FloatRect &_rect) {
            MyGUI::FloatRect r;
            if (!mRenderItem) return;
            MyGUI::ITexture* tex = getTexture();
            if (tex) {
                int left = mCoord.left;
                int top = mCoord.top;
                r.set(float(left)/tex->getWidth(),
                      float(top)/tex->getHeight(),
                      float(left+mCoord.width)/tex->getWidth(),
                      float(top+mCoord.height)/tex->getHeight());
                Base::_setUVSet(r);
            }
        }
        
        void CopySubSkin::_updateView() {
            Base::_updateView();
            _setUVSet(MyGUI::FloatRect());
        }
        
        void CopySubSkin::createDrawItem( MyGUI::ITexture* _texture, MyGUI::ILayerNode* _node) {
            Base::createDrawItem( _texture, _node);
            _setUVSet(MyGUI::FloatRect());
        }
        
        
        void CopySubSkin::doRender(MyGUI::IRenderTarget* _target) {
            if (!mVisible || mEmptyView)
                return;
            
            _target->setTexture(getTexture());
            MyGUI::VertexQuad quad;
            
            fillQuad(quad, _target);
            
            // use premultiplied
            if (mAlign.isLeft()) {
                quad.vertex[MyGUI::VertexQuad::CornerLB].colour =
                quad.vertex[MyGUI::VertexQuad::CornerLB2].colour =
                quad.vertex[MyGUI::VertexQuad::CornerLT].colour = 0x00000000;
            }
            if (mAlign.isRight()) {
                quad.vertex[MyGUI::VertexQuad::CornerRB].colour =
                quad.vertex[MyGUI::VertexQuad::CornerRT].colour =
                quad.vertex[MyGUI::VertexQuad::CornerRT2].colour = 0x00000000;
            }
            if (mAlign.isTop()) {
                quad.vertex[MyGUI::VertexQuad::CornerLT].colour =
                quad.vertex[MyGUI::VertexQuad::CornerRT].colour =
                quad.vertex[MyGUI::VertexQuad::CornerRT2].colour = 0x00000000;
            }
            if (mAlign.isBottom()) {
                quad.vertex[MyGUI::VertexQuad::CornerLB].colour =
                quad.vertex[MyGUI::VertexQuad::CornerRB].colour =
                quad.vertex[MyGUI::VertexQuad::CornerLB2].colour = 0x00000000;
            }
            
            _target->addQuad(quad);
            
        }
        
        
        MaskSetSubSkinState::MaskSetSubSkinState() : m_texture(0) {}
        void MaskSetSubSkinState::deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version) {
            std::string texture = _node->getParent()->findAttribute("texture");
            if (texture.empty()) {
                texture = _node->getParent()->getParent()->findAttribute("texture");
            }
            // tags replacement support for Skins
            if (_version >= MyGUI::Version(1, 1))
            {
                texture = MyGUI::LanguageManager::getInstance().replaceTags(texture);
            }
            
            const MyGUI::IntSize& size = MyGUI::texture_utility::getTextureSize(texture);
            
            m_texture = MyGUI::RenderManager::getInstance().getTexture(texture);
            
            const MyGUI::IntCoord& coord = MyGUI::IntCoord::parse(_node->findAttribute("offset"));
            setRect(MyGUI::CoordConverter::convertTextureCoord(coord, size));
        }
        
        MaskSubSkin::MaskSubSkin() {
            mSeparate = true;
        }
        
        
        void MaskSubSkin::doRender(MyGUI::IRenderTarget* _target) {
            if (!mVisible)
                return;
            if (mRenderItem) {
                {
                    RenderTargetImpl* target = static_cast<RenderTargetImpl*>(_target);
                    MyGUI::Widget* widget_p = static_cast<MyGUI::Widget*>(mCroppedParent);
                    MaskImageWidget* widget = widget_p->castType<MaskImageWidget>(false);
                    
                    ShaderPtr shader;
                    TexturePtr fill_texture;
                    MyGUI::FloatRect fill_texture_uv;
                    
                    if (widget) {
                        ImagePtr    img = widget->getImage();
                        if (img && img->GetTexture()) {
                            fill_texture = img->GetTexture();
                            float itw = 1.0f / img->GetTexture()->GetWidth();
                            float ith = 1.0f / img->GetTexture()->GetHeight();
                            
                            fill_texture_uv.set( img->GetTextureX() * itw,
                                                img->GetTextureY() * ith,
                                                (img->GetTextureX() + img->GetTextureW()) * itw,
                                                (img->GetTextureY() + img->GetTextureH()) * ith);
                        }
                        shader = widget->getShader();
                    }
                    if (!fill_texture) {
                        MyGUI::ISubWidgetRect* main = widget_p->getSubWidgetMain();
                        if (main) {
                            MaskSetSubSkin* sub = main->castType<MaskSetSubSkin>(false);
                            if (sub) {
                                MyGUI::ITexture* tex = sub->getTexture();
                                fill_texture_uv = sub->getUVSet();
                                if (tex) {
                                    fill_texture = static_cast<TextureImpl*>(tex)->GetTexture();
                                }
                            }
                        }
                    }
                    
                    
                    if (!fill_texture) {
                        Base::doRender(_target);
                        return;
                    }
                    
                    Graphics* g = target->graphics();
                    
                    if (g) {
                        g->SetShader(shader);
                        Sandbox::Transform2d mTr = Sandbox::Transform2d();
                        
                        
                        
                        int x = mCroppedParent->getAbsoluteLeft();
                        int y = mCroppedParent->getAbsoluteTop();
                        int w = mCroppedParent->getWidth();
                        int h = mCroppedParent->getHeight();
                        
                        const MyGUI::RenderTargetInfo& info = _target->getInfo();
                        
                        x-=info.leftOffset;
                        y-=info.topOffset;
                        
                        mTr.translate(fill_texture_uv.left,fill_texture_uv.top);
                        mTr.scale(fill_texture_uv.width() / w,fill_texture_uv.height() / h);
                        mTr.translate(-x,-y);
                        
                        g->SetMask(MASK_MODE_ALPHA, fill_texture, mTr);
                        Base::doRender(_target);
                        g->SetMask(MASK_MODE_NONE, TexturePtr(), Transform2d());
                        g->SetShader(ShaderPtr());
                        
                        //
                    } else {
                        sb_assert(false);
                    }
                }
            }
        }
        
        MaskSetSubSkin::MaskSetSubSkin() : m_texture(0) {
            mSeparate = false;
        }
        
        
        
        void MaskSetSubSkin::doRender(MyGUI::IRenderTarget* _target) {
            
        }
        
        void MaskSetSubSkin::setStateData(MyGUI::IStateInfo* _data) {
            m_texture = _data->castType<MaskSetSubSkinState>()->get_texture();
            MyGUI::MainSkin::setStateData(_data);
        }
        
        
        
        void register_skin() {
            MyGUI::FactoryManager& factory = MyGUI::FactoryManager::getInstance();
            
            factory.registerFactory<ColorizedSubSkinStateInfo>(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "ColorizedSubSkin");
                        
            factory.registerFactory<ColorizedSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.registerFactory<CopySubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.registerFactory<MaskSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.registerFactory<MaskSetSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.registerFactory<ObjectSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.registerFactory<AutoSizeText>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.registerFactory<MaskText>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            
            factory.registerFactory<MyGUI::SubSkinStateInfo>(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "MaskSubSkin");
            factory.registerFactory<MaskSetSubSkinState>(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "MaskSetSubSkin");
            factory.registerFactory<MyGUI::SubSkinStateInfo>(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "ObjectSubSkin");
            factory.registerFactory<MyGUI::EditTextStateInfo>(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "AutoSizeText");
            factory.registerFactory<MyGUI::EditTextStateInfo>(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "MaskText");
        }
        
        void unregister_skin() {
            MyGUI::FactoryManager& factory = MyGUI::FactoryManager::getInstance();
            factory.unregisterFactory(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "ColorizedSubSkin");
            factory.unregisterFactory<ColorizedSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.unregisterFactory<CopySubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.unregisterFactory<MaskSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.unregisterFactory<MaskSetSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.unregisterFactory<ObjectSubSkin>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.unregisterFactory<AutoSizeText>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.unregisterFactory<MaskText>(MyGUI::SubWidgetManager::getInstance().getCategoryName());
            factory.unregisterFactory(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "MaskSubSkin");
            factory.unregisterFactory(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "MaskSetSubSkin");
            factory.unregisterFactory(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "ObjectSubSkin");
            factory.unregisterFactory(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "AutoSizeText");
            factory.unregisterFactory(MyGUI::SubWidgetManager::getInstance().getStateCategoryName(), "MaskText");
        }
        
    }
    
}
