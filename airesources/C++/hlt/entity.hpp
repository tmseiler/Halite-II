//
// Created by David Li on 7/17/17.
//

#ifndef ENVIRONMENT_ENTITY_H
#define ENVIRONMENT_ENTITY_H

#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>
#include <vector>

#include "constants.hpp"

namespace hlt {
    /**
     * Uniquely identifies each player.
     */
    typedef unsigned char PlayerId;
    /**
     * Used to identify a ship or planet.
     *
     * Ships are uniquely identified by a combination of the PlayerId of their
     * owner and their EntityIndex. Planets are uniquely identified by their
     * EntityIndex alone.
     */
    typedef unsigned long EntityIndex;

    //! A poor man's std::optional.
    template<typename T>
    using possibly = std::pair<T, bool>;

    struct Velocity {
        double vel_x, vel_y;

        auto accelerate_by(double magnitude, double angle) -> void;
        auto magnitude() const -> double;
        auto angle() const -> double;
    };

    /**
     * A location in Halatian space.
     */
    struct Location {
        double pos_x, pos_y;

        auto distance(const Location& other) const -> double;
        auto distance2(const Location& other) const -> double;

        auto move_by(const Velocity& velocity, double time) -> void;
        auto angle_to(const Location& target) const -> double;

        friend auto operator<< (std::ostream& ostream, const Location& location) -> std::ostream&;
    };

    static bool operator==(const Location& l1, const Location& l2) {
        return l1.pos_x == l2.pos_x && l1.pos_y == l2.pos_y;
    }

    /**
     * Superclass of a ship and a planet. Represents the state of an entity
     * at the beginning of a given turn.
     */
    struct Entity {
        Location location;
        unsigned short health;
        //! The radius of the entity, in terms of units.
        double radius;

        void kill() {
            health = 0;
        }

        bool is_alive() const {
            return health > 0;
        }

        auto heal(unsigned short points) -> void {
            health = std::min(GameConstants::get().MAX_SHIP_HEALTH,
                              static_cast<unsigned short>(health + points));
        }

        /**
         * Return the angle between this entity and the target entity/location.
         * @param target
         * @return
         */
        auto angle_to(const Entity& target) const -> double {
            return this->location.angle_to(target.location);
        }

        auto angle_to(const Location& target) const -> double {
            return this->location.angle_to(target);
        }
    };

    /**
     * The states a ship can be in regarding docking.
     */
    enum class DockingStatus {
        Undocked = 0,
        Docking = 1,
        Docked = 2,
        Undocking = 3,
    };

    struct Planet : Entity {
        PlayerId owner;
        bool owned;

        //! The remaining resources.
        unsigned short remaining_production;
        //! The currently expended resources. A new ship will spawn
        //! once this reaches GameConstants::PRODUCTION_PER_SHIP.
        unsigned short current_production;
        //! The maximum number of ships that may be docked.
        unsigned short docking_spots;

        //! Contains IDs of all ships in the process of docking or undocking,
        //! as well as docked ships.
        std::vector<EntityIndex> docked_ships;

        auto add_ship(EntityIndex ship) -> void;
        auto remove_ship(EntityIndex ship) -> void;
    };

    struct Ship : Entity {
        Velocity velocity;

        //! The turns left before the ship can fire again.
        unsigned int weapon_cooldown;

        DockingStatus docking_status;
        //! The number of turns left to complete (un)docking.
        unsigned int docking_progress;
        //! The index of the planet this ship is docked to. Only valid if
        //! Ship::docking_status is -not- DockingStatus::Undocked.
        EntityIndex docked_planet;

        auto reset_docking_status() -> void;
        auto revive(const Location& loc) -> void;

        /**
         * Check if this ship is close enough to dock to the given planet.
         * @param planet
         * @return
         */
        auto can_dock(const Planet& planet) const -> bool {
            return this->location.distance(planet.location) <=
                GameConstants::get().DOCK_RADIUS + planet.radius + radius;
        }
    };

    /**
     * The type of an entity represented by an entity ID.
     */
    enum class EntityType {
        //! This entity ID does not represent an actual entity.
        InvalidEntity,
        ShipEntity,
        PlanetEntity,
    };

    //! A way to uniquely identify an Entity, regardless of its type.
    struct EntityId {
    private:
        //! Planets are unowned, and have player ID == -1.
        int _player_id;
        int _entity_index;
        EntityId();

    public:
        EntityType type;

        auto is_valid() const -> bool;

        auto player_id() const -> PlayerId;
        auto entity_index() const -> EntityIndex;

        //! Construct an entity ID representing an invalid entity.
        static auto invalid() -> EntityId;
        //! Construct an entity ID for the given planet.
        static auto for_planet(EntityIndex index) -> EntityId;
        //! Construct an entity ID for the given ship.
        static auto for_ship(PlayerId player_id, EntityIndex index) -> EntityId;

        friend auto operator<< (std::ostream& ostream, const EntityId& id) -> std::ostream&;
        friend auto operator== (const EntityId& id1, const EntityId& id2) -> bool;
        friend auto operator!= (const EntityId& id1, const EntityId& id2) -> bool;

        friend struct std::hash<EntityId>;
    };
}

namespace std {
    template<> struct hash<hlt::EntityId> {
    public:
        auto operator()(const hlt::EntityId& id) const -> size_t {
            return static_cast<size_t>(
                (id._entity_index << 8) |
                    (id._player_id & 0xFF));
        }
    };
}

#endif //ENVIRONMENT_ENTITY_H
