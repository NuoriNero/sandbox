#ifndef SB_SPINE_DATA_H_INCLUDED
#define SB_SPINE_DATA_H_INCLUDED

#include "meta/sb_meta.h"
#include <sbstd/sb_map.h>
#include <sbstd/sb_string.h>
#include "sb_draw_attributes.h"

struct spAtlas;
struct spSkeletonData;
struct spAnimationStateData;

namespace Sandbox {
    
    class Resources;
    class SpineSkeleton;
    class SpineData;
    typedef sb::intrusive_ptr<SpineData> SpineDataPtr;
    
    class SpineAnimation;
    typedef sb::intrusive_ptr<SpineAnimation> SpineAnimationPtr;
    
    class SpineData : public meta::object {
        SB_META_OBJECT
    public:
        static SpineDataPtr Load(const char* atlas_file,
                                 const char* skeleton_file,
                                 Resources* resources);
        ~SpineData();
        
        void SetDefaultMix(float duration);
        void SetMix(const char* from, const char* to, float duration);
        size_t GetSlotsCount() const;
        sb::string GetSlotName(size_t idx) const;
        void SetSlotAttribute(size_t idx, const DrawAttributesPtr& attribute);
        const DrawAttributesPtr& GetSlotAttribute(const void* idx) const;
    private:
        SpineData();
        spAtlas*   m_atlas;
        spSkeletonData* m_skeleton;
        spAnimationStateData* m_state;
        friend class SpineAnimation;
        sb::map<const void*,DrawAttributesPtr> m_attributes;
    };
    
}

#endif /*SB_SPINE_DATA_H_INCLUDED*/
