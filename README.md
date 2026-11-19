# Maquina-Mealy-Refri-Bitdoglab
maquina-mealy-refri-bitdoglab

Para atualizar o firmware de forma persistente, execute a tarefa **Run Project** no VS Code.
Ela compila o projeto e usa o `picotool` para gravar a imagem na flash, verificar a gravação
e reiniciar a BitDogLab. Executar apenas **Compile Project** não grava o firmware na placa.
