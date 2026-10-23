import unreal

MAP_PACKAGE_PATH = "/Game/MainMenu/Maps/L_MainMenu"
GAMEMODE_CLASS_PATH = "/Script/SurfGame.MainMenuGameMode"


def main():
    unreal.log("[MainMenu] Iniciando criacao do level L_MainMenu...")

    gamemode_class = unreal.load_class(None, GAMEMODE_CLASS_PATH)
    if not gamemode_class:
        raise RuntimeError(f"[MainMenu] Erro: Classe {GAMEMODE_CLASS_PATH} nao encontrada!")

    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    # Cria um mapa totalmente vazio em /Game/MainMenu/Maps/L_MainMenu
    created = level_subsystem.new_level(MAP_PACKAGE_PATH)
    if not created:
        world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    else:
        world = editor_subsystem.get_editor_world()

    if not world:
        raise RuntimeError("[MainMenu] Erro: Nao foi possivel obter o EditorWorld para L_MainMenu.")

    world_settings = world.get_world_settings()
    if not world_settings:
        raise RuntimeError("[MainMenu] Erro: Nao foi possivel obter o WorldSettings do level.")

    world_settings.set_editor_property("default_game_mode", gamemode_class)

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP_PACKAGE_PATH)
    if not saved:
        saved = level_subsystem.save_current_level()

    if not saved:
        raise RuntimeError(f"[MainMenu] Erro ao salvar o mapa em {MAP_PACKAGE_PATH}.")

    unreal.log(f"[MainMenu] Level {MAP_PACKAGE_PATH} criado e salvo com sucesso (GameModeOverride={gamemode_class.get_path_name()}).")


if __name__ == "__main__":
    main()
