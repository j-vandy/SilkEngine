#pragma once

#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>
#include <cassert>

namespace silk
{
    using Entity = uint32_t;
    inline uint32_t nextComponentTypeID = 0;

    template<typename T>
    uint32_t getComponentTypeID()
    {
        static const uint32_t typeID = nextComponentTypeID++;
        return typeID;
    }

    struct BaseComponentPool
    {
        virtual ~BaseComponentPool() = default;
        virtual void remove(Entity e) = 0;
        virtual bool has(Entity e) const = 0;
    };

    template <typename T>
    struct ComponentPool : BaseComponentPool
    {
        std::vector<T> components;
        std::vector<Entity> owners;
        std::unordered_map<Entity, uint32_t> entityToComponentIndex;

        void remove(Entity e) override
        {
            assert(has(e));

            uint32_t deletedIdx = entityToComponentIndex[e];
            uint32_t lastIdx = static_cast<uint32_t>(components.size() - 1);

            // copy last index into deleted index for density
            components[deletedIdx] = components[lastIdx];

            Entity movedEntity = owners[lastIdx];
            owners[deletedIdx] = movedEntity;
            entityToComponentIndex[movedEntity] = deletedIdx;

            components.pop_back();
            owners.pop_back();
            entityToComponentIndex.erase(e);
        }

        bool has(Entity e) const override
        {
            return entityToComponentIndex.count(e) > 0;
        }
    };

    class Scene
    {
    public:
        template <typename T>
        void addComponent(Entity e, T component)
        {
            ComponentPool<T>& pool = getComponentPool<T>();
            assert(!pool.has(e));
            pool.entityToComponentIndex[e] = static_cast<uint32_t>(pool.components.size());
            pool.owners.push_back(e);
            pool.components.push_back(component);
        }

        template <typename T>
        bool hasComponent(Entity e)
        {
            const ComponentPool<T>& pool = getComponentPool<T>();
            return pool.has(e);
        }

        template <typename T>
        T& getComponent(Entity e)
        {
            ComponentPool<T>& pool = getComponentPool<T>();
            assert(pool.has(e));
            return pool.components[pool.entityToComponentIndex[e]];
        }

        template <typename T>
        void removeComponent(Entity e)
        {
            ComponentPool<T>& pool = getComponentPool<T>();
            pool.remove(e);
        }
    
        template <typename... T>
        Entity createEntity(T&&... components)
        {
            Entity e;
            if (!freedEntities.empty())
            {
                e = freedEntities.top();
                freedEntities.pop();
            }
            else
            {
                e = nextEntityID++;
            }

            (addComponent(e, std::forward<T>(components)), ...);

            return e;
        }

        void deleteEntity(Entity e)
        {
            for (auto& pool : componentPools)
            {
                if (pool && pool->has(e))
                {
                    pool->remove(e);
                }
            }
            freedEntities.push(e);
        }

        template <typename... T>
        std::vector<Entity> query()
        {
            std::vector<Entity> entities;
            const auto& baseOwners = getComponentPool<typename std::tuple_element<0, std::tuple<T...>>::type>().owners;

            for (Entity e : baseOwners)
            {
                if ((hasComponent<T>(e) && ...))
                {
                    entities.push_back(e);
                }
            }

            return entities;
        }

    private:
        std::stack<Entity> freedEntities;
        Entity nextEntityID = 0;
        std::vector<std::unique_ptr<BaseComponentPool>> componentPools;

        template <typename T>
        inline ComponentPool<T>& getComponentPool()
        {
            const uint32_t componentTypeID = getComponentTypeID<T>();

            if (componentTypeID >= static_cast<uint32_t>(componentPools.size()))
            {
                componentPools.resize(componentTypeID + 1);
                componentPools[componentTypeID] = std::make_unique<ComponentPool<T>>();
            }

            return *static_cast<ComponentPool<T>*>(componentPools[componentTypeID].get());
        }
    };
}