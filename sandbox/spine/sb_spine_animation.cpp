#include "sb_spine_animation.h"
#include "sb_spine_data.h"
#include "sb_graphics.h"
#include <spine/spine.h>

SB_META_DECLARE_OBJECT(Sandbox::SpineAnimation, Sandbox::Thread)
SB_META_DECLARE_OBJECT(Sandbox::SpineSceneObject, Sandbox::Container)

namespace Sandbox {
    
    void spine_animation_event_listener(spAnimationState* state, int trackIndex, spEventType type, spEvent* event,
                                        int loopCount);
    
    SpineAnimation::SpineAnimation(const SpineDataPtr& data) : m_data(data) {
        m_block_events = false;
        m_skeleton = spSkeleton_create(m_data->m_skeleton);
        m_state = spAnimationState_create(m_data->m_state);
        spSkeleton_setToSetupPose(m_skeleton);
        m_state->rendererObject = this;
        m_state->listener = &spine_animation_event_listener;
        m_last_animation = 0;
    }
    SpineAnimation::~SpineAnimation() {
        if (m_skeleton) {
            spSkeleton_dispose(m_skeleton);
        }
        if (m_state) {
            spAnimationState_dispose(m_state);
        }
    }
    
    void SpineAnimation::StartAnimation(const char* name,int loop) {
        spAnimation* animation = spSkeletonData_findAnimation(m_state->data->skeletonData, name);
        if (!animation) {
            LogError() << "not found animation " << name;
            return;
        }
        if (!spAnimationState_getCurrent(m_state,0)) {
            spSkeleton_setToSetupPose(m_skeleton);
        }
        spAnimationState_setAnimation(m_state, 0, animation, loop == 0 ? 1 : 0);
        if (loop!=0) {
            for (int i=1;i<loop;++i) {
                spAnimationState_addAnimation(m_state, 0, animation, 0, 0.0f);
            }
        }
    }
    
    void SpineAnimation::SetAnimation(const char* name) {
        spAnimation* animation = spSkeletonData_findAnimation(m_state->data->skeletonData, name);
        if (!animation) {
            LogError() << "not found animation " << name;
            return;
        }
        spSkeleton_setToSetupPose(m_skeleton);
        spAnimationState_setAnimation(m_state, 0, animation, 0);
    }
    void SpineAnimation::AddAnimation(const char* name,int loop) {
        spAnimation* animation = spSkeletonData_findAnimation(m_state->data->skeletonData, name);
        if (!animation) {
            LogError() << "not found animation " << name;
            return;
        }
        if (!spAnimationState_getCurrent(m_state,0)) {
            spSkeleton_setToSetupPose(m_skeleton);
            spAnimationState_setAnimation(m_state, 0, animation, loop == 0 ? 1 : 0);
        } else {
            spAnimationState_addAnimation(m_state, 0, animation, loop == 0 ? 1 : 0 , 0.0f);
        }
        if (loop!=0) {
            for (int i=1;i<loop;++i) {
                spAnimationState_addAnimation(m_state, 0, animation, 0, 0.0f);
            }
        }
    }
    
    bool SpineAnimation::HasAnimation(const char* name) const {
        spAnimation* animation = spSkeletonData_findAnimation(m_state->data->skeletonData, name);
        return animation != 0;
    }
    
    Color SpineAnimation::GetSlotColor(const char* slot_name) const {
        spSlot* slot = spSkeleton_findSlot(m_skeleton, slot_name);
        if (slot) {
            return Color(slot->r,slot->g,slot->b,slot->a);
        }
        return Color();
    }
    
    void SpineAnimation::SetTime( float time ) {
        spTrackEntry* entry = spAnimationState_getCurrent (m_state, 0);
        if (entry && entry->animation) {
            float current = entry->time;
            float dt = time - current;
            m_block_events = dt <= 0.0f;
            Update(dt);
            m_block_events = false;
        }
    }
    float SpineAnimation::GetCurrentAnimationLength() const {
        spTrackEntry* entry = spAnimationState_getCurrent (m_state, 0);
        if (entry && entry->animation) {
            return entry->animation->duration;
        }
        return 0;
    }
    
    bool SpineAnimation::IsComplete() const {
        spTrackEntry* entry = spAnimationState_getCurrent (m_state, 0);
        if (entry && entry->animation) {
            return false;
        }
        return true;
    }
    
    
    bool SpineAnimation::Update(float dt) {
        spSkeleton_update(m_skeleton, dt);
        spAnimationState_update(m_state, dt);
        spAnimationState_apply(m_state, m_skeleton);
        spSkeleton_updateWorldTransform(m_skeleton);
        return false;
    }
    
    void SpineAnimation::OnAnimationComplete() {
        if (m_end_signal) {
            m_end_signal->Emmit();
        }
    }
    
    void SpineAnimation::Clear() {
        if (m_end_signal) {
            m_end_signal->Clear();
            m_end_signal.reset();
        }
        if (m_event_signal) {
            m_event_signal->Clear();
            m_event_signal.reset();
        }
        Thread::Clear();
    }
    
    void SpineAnimation::OnAnimationStarted() {
        spSkeleton_setToSetupPose(m_skeleton);
        spAnimationState_apply(m_state, m_skeleton);
        spSkeleton_updateWorldTransform(m_skeleton);
    }
    
    void SpineAnimation::OnAnimationEvent(spEvent* e) {
        if (m_data && m_event_signal && !m_block_events) {
            const EventPtr& ae = m_data->GetEvent(e->data);
            if (ae) {
                m_event_signal->Emmit(ae);
            }
        }
    }
    void spine_animation_event_listener(spAnimationState* state, int trackIndex, spEventType type, spEvent* event,
                                        int loopCount) {
        if (state && state->rendererObject) {
            if (type == SP_ANIMATION_COMPLETE) {
                static_cast<SpineAnimation*>(state->rendererObject)->OnAnimationComplete();
            } else if (type == SP_ANIMATION_START) {
                static_cast<SpineAnimation*>(state->rendererObject)->OnAnimationStarted();
            }  else if (type == SP_ANIMATION_EVENT) {
                static_cast<SpineAnimation*>(state->rendererObject)->OnAnimationEvent(event);
            }
        }
    }
    
    Rectf SpineAnimation::CalcBoundingBox() const {
        spSkeletonBounds* bounds = spSkeletonBounds_create();
        spSkeletonBounds_update(bounds, m_skeleton, true);
        Rectf r;
        if (bounds->count) {
            r.x = bounds->minX;
            r.y = bounds->minY;
            r.w = bounds->maxX - bounds->minX;
            r.h = bounds->maxY - bounds->minY;
            spSkeletonBounds_dispose(bounds);
        } else {
            float vert[8];
            bool first = true;
            for (int i = 0; i < m_skeleton->slotsCount; ++i) {
                spSlot* slot = m_skeleton->drawOrder[i];
                if (!slot)
                    continue;
                if (!slot->bone)
                    continue;
                if (slot->a <= 0.01f)
                    continue;
                spAttachment* attachment = slot->attachment;
                if (!attachment || attachment->type != SP_ATTACHMENT_REGION) continue;
                spRegionAttachment* ra = (spRegionAttachment*)attachment;
                spRegionAttachment_computeWorldVertices(ra,slot->bone,vert);
                if (first) {
                    r.x = vert[SP_VERTEX_X1];
                    r.y = vert[SP_VERTEX_Y1];
                    r.w = 0;
                    r.h = 0;
                    first = false;
                } else {
                    r.Extend(Vector2f(vert[SP_VERTEX_X1],vert[SP_VERTEX_Y1]));
                }
                r.Extend(Vector2f(vert[SP_VERTEX_X2],vert[SP_VERTEX_Y2]));
                r.Extend(Vector2f(vert[SP_VERTEX_X3],vert[SP_VERTEX_Y3]));
                r.Extend(Vector2f(vert[SP_VERTEX_X4],vert[SP_VERTEX_Y4]));
            }
        }
        return r;
    }
    
    
    
    SpineSceneAttachement::SpineSceneAttachement(const sb::string& attachement) : m_attachement(attachement) {
        
    }
    
    void SpineSceneAttachement::GetTransformImpl(Transform2d& tr) const {
        Container::GetTransformImpl(tr);
        if (!GetParent())
            return;
        SpineSceneObject* parent = meta::sb_dynamic_cast<SpineSceneObject>(GetParent());
        if (parent) {
            parent->ApplySlotTransform(tr,m_attachement);
        }
    }
    
    SpineSceneObject::SpineSceneObject(const SpineAnimationPtr& animation) : m_animation(animation) {
        
    }
    
    void SpineSceneObject::SetAttachement(const sb::string& slot_name,const SceneObjectPtr& object) {
        AttachementMap::iterator it = m_attachements.find(slot_name);
        if (it!=m_attachements.end()) {
            RemoveObject(it->second);
        }
        m_attachements[slot_name] = SpineSceneAttachementPtr(new SpineSceneAttachement(slot_name));
        m_attachements[slot_name]->AddObject(object);
        AddObject(m_attachements[slot_name]);
    }
    void SpineSceneObject::RemoveAttachement(const sb::string& slot_name) {
        AttachementMap::iterator it = m_attachements.find(slot_name);
        if (it!=m_attachements.end()) {
            RemoveObject(it->second);
            m_attachements.erase(it);
        }
    }
    
    void SpineSceneObject::ApplySlotTransform(Transform2d& atr,const sb::string& slot_name) const {
        if (!m_animation)
            return;
        spSkeleton* skeleton = m_animation->m_skeleton;
        for (int i = 0; i < skeleton->slotsCount; ++i) {
            spSlot* slot = skeleton->drawOrder[i];
            if (!slot)
                continue;
            spBone* bone = slot->bone;
            if (slot_name == slot->data->name) {
                Sandbox::Transform2d tr;
                tr.m.matrix[0*2+0] = bone->a;
                tr.m.matrix[0*2+1] = bone->c;
                tr.m.matrix[1*2+0] = bone->b;
                tr.m.matrix[1*2+1] = bone->d;
                tr.v.x = skeleton->x + bone->worldX;
                tr.v.y = skeleton->y + bone->worldY;
                
                atr = atr * tr;
                return;
            }
        }
    }
    void SpineSceneObject::Draw(Graphics &gr) const {
        if (!m_animation)
            return;
        Transform2d str = gr.GetTransform();
        Color clr = gr.GetColor();
        BlendMode bm = gr.GetBlendMode();
        
        if (GetTransformM()) {
            GetTransformM()->Apply(gr);
        }
        if (GetColorM()) {
            GetColorM()->Apply(gr);
        }

        
        spSkeleton* skeleton = m_animation->m_skeleton;
        for (int i = 0; i < skeleton->slotsCount; ++i) {
            spSlot* slot = skeleton->drawOrder[i];
            if (!slot)
                continue;
            spBone* bone = slot->bone;
            spAttachment* attachment = slot->attachment;
            SpineSceneAttachementPtr object_attachement;
            {
                AttachementMap::const_iterator it = m_attachements.find(slot->data->name);
                if (it!=m_attachements.end()) {
                    object_attachement = it->second;
                }
            }
            if (attachment && attachment->type != SP_ATTACHMENT_REGION) {
                attachment = 0;
            }
            if (!attachment && !object_attachement) continue;
            
            switch (slot->data->blendMode) {
                case SP_BLEND_MODE_ADDITIVE:
                    gr.SetBlendMode(BLEND_MODE_ADDITIVE);
                    break;
                case SP_BLEND_MODE_MULTIPLY:
                    gr.SetBlendMode(BLEND_MODE_MULTIPLY);
                    break;
                case SP_BLEND_MODE_SCREEN:
                    gr.SetBlendMode(BLEND_MODE_SCREEN);
                    break;
                default:
                    gr.SetBlendMode(BLEND_MODE_ALPHABLEND);
                    break;
            }
            
            Sandbox::Color c(skeleton->r*slot->r,skeleton->g*slot->g,skeleton->b*slot->b,skeleton->a*slot->a);
            if (c.a <= 0.0f)
                continue;
            c.clamp();
            gr.SetColor(clr*c);
            
            Sandbox::Transform2d tr;
            tr.m.matrix[0*2+0] = bone->a;
            tr.m.matrix[0*2+1] = bone->c;
            tr.m.matrix[1*2+0] = bone->b;
            tr.m.matrix[1*2+1] = bone->d;
            tr.v.x = skeleton->x + bone->worldX;
            tr.v.y = skeleton->y + bone->worldY;
            
            if (attachment)
            {
                
                spRegionAttachment* ra = (spRegionAttachment*)attachment;
                spAtlasRegion* region = (spAtlasRegion*)ra->rendererObject;
                TexturePtr tex(static_cast<Texture*>(region->page->rendererObject));
                if (!tex) continue;
                
                Image img(tex,float(region->x),
							float(region->y),
                          float(region->rotate ? region->height : region->width),
                          float(region->rotate ? region->width : region->height));
                if (region->rotate) {
                    img.SetRotated(true);
                    img.SetSize(float(region->width),float(region->height));
                }
                float oy = float(region->originalHeight-region->height-region->offsetY);
                img.SetHotspot(Vector2f(float(region->originalWidth / 2 - region->offsetX),
					float(region->originalHeight / 2 - oy)));
                
                float regionScaleX = ra->width / ra->regionOriginalWidth * ra->scaleX;
                float regionScaleY = ra->height / ra->regionOriginalHeight * ra->scaleY;
                
                float radians = ra->rotation * float(M_PI) / 180.0f;
                Sandbox::Transform2d trAttachment;
                trAttachment.translate(ra->x,ra->y);
                trAttachment.rotate(radians);
                trAttachment.scale(regionScaleX,-regionScaleY);
                tr =  tr * trAttachment;
                
                gr.SetTransform(str * tr);
                const DrawAttributesPtr& attr = m_animation->m_data->GetSlotAttribute(slot->data);
                gr.DrawImage(img,attr.get(),0,0);
                
            }
            if (object_attachement) {
                gr.SetTransform(str * tr);
                object_attachement->Draw(gr);
            }
        }
        gr.SetColor(clr);
        gr.SetTransform(str);
        gr.SetBlendMode(bm);
    }
}
