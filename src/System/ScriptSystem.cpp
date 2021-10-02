#include "ScriptSystem.h"

#include <luaaa.hpp>

#include <Aka/OS/Logger.h>
#include <Aka/Core/Container/String.h>

namespace viewer {

struct ScriptComponent
{
    aka::String name;
    aka::String script; // whole script.
};

static lua_State* L = nullptr;

struct Cat
{
    Cat() :
        Cat("") {}
    Cat(const std::string& name) :
        name(name)
    {
    }
    std::string getName()
    {
        return name;
    }
    void setName(const std::string& string)
    {
        name = string;
    }
    ~Cat()
    {
    }
private:
    std::string name;
};

#define LUA_CHECK_RESULT(res) { int result = (res); if (result != LUA_OK) { \
size_t len = 0;\
aka::Logger::error("[script]", AKA_STRINGIFY(res), luaL_tolstring(L, -1, &len));\
lua_pop(L, 1);\
} } \

// Override lua logging (stdout) to use aka logger
static int l_my_print(lua_State* L)
{
    std::string str;
    int n = lua_gettop(L);  // number of arguments
    for (int i = 1; i <= n; i++) {  // for each argument
        size_t l;
        const char* s = luaL_tolstring(L, i, &l);  // convert it to string 
        if (i > 1)  // not the first element? 
            str.append("\t");
        str.append(s, l);
        lua_pop(L, 1);  // pop result 
    }
    aka::Logger::debug("[script]", str);
    return 0;
}

void createEntity(lua_State* state)
{
    // do something
}

void destroyEntity(lua_State* state)
{
    // do something
}

static const struct luaL_Reg printlib[] = {
    {"print", l_my_print},
    {NULL, NULL} // end of array
};

struct LuaValue {
    int type;
    union {
        lua_Number number;
        int boolean;
        const char* string;
    };
};

void registerScript(const char* name, const char* script)
{
    if (lua_getglobal(L, name) != LUA_TNIL)
    {
        aka::Logger::warn("Name already used for script.");
        return;
    }
    // https://stackoverflow.com/questions/63643025/lua-5-2-sandboxing-in-different-objects-with-c-api
    lua_newtable(L); // Create a table / TBL
    LUA_CHECK_RESULT(luaL_loadstring(L, script)); // Load chunk on stack / TBL - CHNK
    lua_newtable(L); // Create metatable / TBL - CHNK - TBL(mt)

    // Allow _G to be accessed if nothing found in table
    int type = lua_getglobal(L, "_G"); // Get the global state / TBL - CHNK - TBL(mt) - _G
    lua_setfield(L, 3, "__index"); // Set field of meta table __index at index 3 to be _G
    int dummy = lua_setmetatable(L, 1); // Link metatable at top of stack to table at index 1
    lua_pushvalue(L, 1); // Copy the table and push it to the top of the stack

    const char* n = lua_setupvalue(L, -2, 1); // Assign to _ENV
    LUA_CHECK_RESULT(lua_pcall(L, 0, 0, 0)); // Compile chunk
    lua_setglobal(L, name); // Empty the stack
    AKA_ASSERT(lua_gettop(L) == 0, "Stack not empty");
}

void unregisterScript(const char* name)
{
    // Set env as nil. GC should remove all references to the script env.
    lua_pushnil(L);
    lua_setglobal(L, name);
}

bool executeScriptFunction(const char* scriptName, const char* name, const LuaValue* args, int argc)
{
    int outArgc = 0;
    std::vector<LuaValue> outArgs(outArgc);
    lua_getglobal(L, scriptName); // Get the script env
    if (lua_isnil(L, 1) != 0)
        return false; // no script env found

    // Load function to the stack
    int type = lua_getfield(L, 1, name);
    if (type == LUA_TFUNCTION)
    {
        for (int i = 0; i < argc; i++)
        {
            switch (args[i].type)
            {
            case LUA_TNUMBER:
                lua_pushnumber(L, args[i].number);
                break;
            case LUA_TBOOLEAN:
                lua_pushboolean(L, args[i].boolean);
                break;
            case LUA_TSTRING:
                lua_pushstring(L, args[i].string);
                break;
            default:
                lua_settop(L, 0);
                aka::Logger::error("Failed to parse string");
                AKA_ASSERT(lua_gettop(L) == 0, "Stack not empty");
                return false;
            }
        }
        LUA_CHECK_RESULT(lua_pcall(L, argc, outArgc, 0));
        for (int n = 0; n < outArgc; n++)
        {
            LuaValue value;
            value.type = type;
            switch (type)
            {
            case LUA_TNUMBER:
                value.number = lua_tonumber(L, -1);
                break;
            case LUA_TBOOLEAN:
                value.boolean = lua_toboolean(L, -1);
                break;
            case LUA_TSTRING:
                value.string = lua_tostring(L, -1);
                break;
            default:
                value.string = nullptr;
                aka::Logger::warn("Invalid output type");
                break;
            }
            lua_pop(L, 1);
        }
        // Execute GC. Only on update ?
        LUA_CHECK_RESULT(lua_gc(L, LUA_GCCOLLECT, 0));
        AKA_ASSERT(lua_gettop(L) == 0, "Stack not empty");
        return true;
    }
    else
    {
        // No function found with the given name
        lua_pop(L, 1);
        AKA_ASSERT(lua_gettop(L) == 0, "Stack not empty");
        return false;
    }
}

void onScriptConstruct(entt::registry& registry, entt::entity entity)
{
    // Script not set yet
    ScriptComponent& script = registry.get<ScriptComponent>(entity);
    registerScript(script.name.cstr(), script.script.cstr());
    executeScriptFunction(script.name.cstr(), "onCreate", nullptr, 0);
}

void onScriptDestruct(entt::registry& registry, entt::entity entity)
{
    ScriptComponent& script = registry.get<ScriptComponent>(entity);
    executeScriptFunction(script.name.cstr(), "onDestroy", nullptr, 0);
    unregisterScript(script.name.cstr());
}

void ScriptSystem::onCreate(aka::World& world)
{
    { // Create Lua state
        L = luaL_newstate();
        // We can add custom libs here.
        luaL_openlibs(L);
    }

    { // Register custom logger
        int type = lua_getglobal(L, "_G");
        luaL_setfuncs(L, printlib, 0);
        lua_pop(L, 1);
    }

    //lua_atpanic(L, [](lua_State* L) -> int {
    //    return 0; // longjump to avoid abort.
    //});

    {
        world.registry().on_construct<ScriptComponent>().connect<&onScriptConstruct>();
        world.registry().on_destroy<ScriptComponent>().connect<&onScriptDestruct>();
    }

    // TODO register all components ?
    // When we call new, the component is added to the entity bound by the script ? calling world.create()
    // We have entity script, bound to an entity with ScriptComponent
    // We have scene script, bound to the scene, who can loop through entities ?
    
    // Register custom classes
    /*luaaa::LuaClass<Cat> luaCat(L, "AwesomeCat");
    luaCat.ctor<std::string>();
    luaCat.fun("getName", &Cat::getName);
    luaCat.fun("setName", &Cat::setName);
    luaCat.def("tag", "Cat");

    // Register global class
    luaaa::LuaModule global(L);
    global.def("pi", aka::pi<float>.radian());
    global.def("dict", std::set<std::string>({ "cat", "dog", "cow" }));

    global.def("aka", std::map<std::string, std::string>({ {"cat", ""}, {"dog", ""}, {"cow", ""} }));

    // Register modulesclass
    luaaa::LuaModule moduled(L, "world");
    moduled.fun("create", createEntity);
    moduled.def("destroy", destroyEntity);*/

    { // Dummy
        // Load script from file
        aka::String str;
        aka::Entity e0 = world.createEntity("script0");
        aka::File::read(aka::ResourceManager::path("scripts/test.lua"), &str);
        e0.add<ScriptComponent>(ScriptComponent{ "test", str });

        aka::Entity e1 = world.createEntity("script1");
        aka::File::read(aka::ResourceManager::path("scripts/test2.lua"), &str);
        e1.add<ScriptComponent>(ScriptComponent{ "test2", str });
    }
}

void ScriptSystem::onDestroy(aka::World& world)
{
    world.registry().on_construct<ScriptComponent>().disconnect<&onScriptConstruct>();
    world.registry().on_destroy<ScriptComponent>().disconnect<&onScriptDestruct>();
    lua_close(L);
}

void ScriptSystem::onUpdate(aka::World& world, aka::Time::Unit deltaTime)
{
    auto view = world.registry().view<ScriptComponent>();
    view.each([&](ScriptComponent& script) {
        LuaValue value;
        value.type = LUA_TNUMBER;
        value.number = deltaTime.seconds();
        executeScriptFunction(script.name.cstr(), "onUpdate", &value, 1);
        
        /*// TOD load the values prior to update and run them
        // Load script on the stack
        LUA_CHECK_RESULT(luaL_loadstring(L, script.script.cstr()));
        // Run the script once to initiate its global variables.
        LUA_CHECK_RESULT(lua_pcall(L, 0, 0, 0));
        // Load function to the stack
        int type = lua_getglobal(L, "onUpdate");
        if (type == LUA_TFUNCTION)
        {
            // push function arguments into stack
            lua_pushnumber(L, deltaTime.seconds());
            LUA_CHECK_RESULT(lua_pcall(L, 1, 0, 0));
            // If there are return values, they are pushed onto the stack.
            // Run GC
            LUA_CHECK_RESULT(lua_gc(L, LUA_GCCOLLECT, 0));
        }
        else
        {
            lua_pop(L, 1);
        }*/
    });
}

}