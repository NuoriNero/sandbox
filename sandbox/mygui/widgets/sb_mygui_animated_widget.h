//
//  sb_mygui_animated_widget.h
//  Sandbox
//
//
//

#ifndef _SB_MYGUI_ANIMATED_WIDGET_H_INCLUDED_
#define _SB_MYGUI_ANIMATED_WIDGET_H_INCLUDED_

#include "MyGUI_Widget.h"
#include "meta/sb_meta.h"
#include "sb_vector2.h"
#include "sb_container.h"
#include "MyGUI_SharedLayerNode.h"
#include "sb_threads_mgr.h"

namespace Sandbox {
    
    namespace mygui {
        class RenderTargetImpl;
        class AnimatedLayerNode;
        
        class AnimatedWidget : public MyGUI::Widget {
            MYGUI_RTTI_DERIVED( AnimatedWidget )
        public:
            AnimatedWidget();
            ~AnimatedWidget();
            
            virtual MyGUI::ILayerNode* createChildItemNode(MyGUI::ILayerNode* _node);
            
            const TransformModificatorPtr& GetTransform() const;
            const ColorModificatorPtr& GetColor() const;
            const ThreadsMgrPtr& GetThread() const { return m_thread; }
            
            void SetOrigin(const Vector2f& o);
            const Vector2f& GetOrigin() const { return m_origin; }
            
            void renderToTarget(MyGUI::IRenderTarget* rt,AnimatedLayerNode* node,bool update);
            virtual void DrawContent(MyGUI::IRenderTarget* rt,AnimatedLayerNode* node,bool update);
            
        protected:
            void initialiseOverride();
            void shutdownOverride();
            
            virtual void setPropertyOverride(const std::string& _key, const std::string& _value);
            
            mutable TransformModificatorPtr     m_transform;
            mutable ColorModificatorPtr         m_color;
            ThreadsMgrPtr               m_thread;
            Vector2f            m_origin;
         private:
            void frameEntered(float dt);
        };
        
    }
    
}

#endif /* _SB_MYGUI_ANIMATED_WIDGET_H_INCLUDED_ */