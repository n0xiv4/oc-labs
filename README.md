# Relatório sobre o LAB1 de OC - Hierarquia de memória (caches)

## Direct-Mapped L1 Cache

Na primeira tarefa, foi nos pedido para que adaptássemos o código existente (`SimpleCache.c`) para uma *direct-mapped L1 Cache*. Ao analisar o código, percebemos que a nossa estrutura `Cache` terá que sofrer alterações: cada `Cache` apenas continha uma `CacheLine` (`CacheLine line;`), o que não corresponde naturalmente ao funcionamento do que nos é pedido. Cria-se então um array de `CacheLine`, de tamanho `L1_SIZE / BLOCK_SIZE`, o que equivale ao número de linhas da `Cache`. 

Posteriormente, requerem-se outras alterações: considerando o número de linhas descoberto previamente, e o tamanho das mesmas (`BLOCK_SIZE`), é possível calcular o número de bits do nosso endereço que serão utilizados para o `offset`, e para o `index`. Sendo que `BLOCK_SIZE = 64 = 2^6` e que `L1_SIZE / BLOCK_SIZE = 256 = 2^8`, sabemos que o nosso endereço será composto por 6 bits de `offset`, 8 bits de `index` e 18 bits de `tag`. 

No código existente, não há referências quanto ao `offset` nem ao `index`; subentende-se pela leitura que consoante seja uma palavra par ou ímpar (a nível de endereço), colocamos a palavra no endereço `0` ou `WORD_SIZE` da `Cache`, respetivamente. Naturalmente, isto é algo a mudar na nova implementação - terá bastante mais sentido utilizar `index * BLOCK_SIZE + offset` como posição em que colocar a palavra; `index * BLOCK_SIZE` porque a primeira linha irá de `0` a `BLOCK_SIZE`, e assim sucessivamente para cada linha; `+ offset` para a posição da palavra na própria linha.

Outra área que necessitou de alterações foi a do tratamento dos chamados *dirty blocks*: como no código existente apenas existia uma linha, era possível encontrar o endereço do bloco atual na memória apenas utilizando a tag (`MemAddress = Line->Tag << 3;`). Isto deixa de ser possível a partir do momento em que temos mais do que uma linha; e passamos a ter que adicionar o `index` a este endereço. O `offset` é desnecessário - estamos a tratar de todo o bloco.

## Direct-Mapped L2 Cache

Na segunda tarefa, foi nos pedida uma implementação de uma *cache L2*. Utilizando o código já existente em `L1Cache.c` (da última tarefa, portanto), foi mais fácil chegar a esta implementação; na função `accessL1`, foi necessário entender que o tratamento de *dirty blocks* estaria agora associado com a função `accessL2` e não com a função `accessDRAM`, uma vez que o seguinte nível de memória passaria agora a ser a cache L2.

É também relevante mencionar que uma vez que `L2_SIZE = 2 * L1_SIZE`, e que `BLOCK_SIZE` se mantém, então o número de linhas na cache L2 será o dobro do número de linhas na cache L1. Isto significa, a nível de `index`, a necessidade de mais 1 bit - passam então a ser 9.

## 2-Way Set Associative L2 Cache

Na terceira tarefa, foi nos pedida uma implementação de uma *two-way set associative L2 cache*. Inicialmente, podemos pensar que teremos *sets* de 2 linhas, o que implica que o número de *sets* será metade do número de linhas. Isto reverte a necessidade de mais 1 bit de que falámos na tarefa anterior; o `index` representa neste caso o `set`. Posto isto, uma vez que mantemos a estrutura `Cache` vinda de `L2Cache.h`, teremos que arranjar uma maneira de localizar uma linha considerando o `set` em que esta se encontra. Utilizamos então a expressão `line_index = 2 * set_index + set_line;` - que permite que as linhas de cada `set` estejam localizadas em regiões seguidas do array `L2Cache`.

Teremos que repensar as situações de *hit* e de *miss* - considerando que agora teremos 2 linhas, não nos basta utilizar um `if` do género `if (!Line->Valid || Line->Tag != Tag)`. Teremos então várias verificações a fazer:
1. Ver se cada linha do `set` tem a mesma `tag` do endereço pedido, se sim, temos *hit* nessa linha;
2. Caso contrário, estamos perante um *miss*, e devemos perceber se ambas as linhas do `set` estão `Valid`; caso exista alguma que não esteja, poderemos utilizá-la;
3. Caso nada disto se verifique, encontramos a linha do `set` cujo `Time` é inferior (seguindo a *Least Recently Used policy* pedida), e utilizamos essa.

## Testes

Decidimos, a nível de testes, incidir na última tarefa mais em específico. Considerando que temos uma *two-way set associative L2 cache*, faz sentido testar vários endereços que dêem conflito. Encontrámos então endereços que tivessem o mesmo `index`, mas `tag`'s diferentes. Para simplificar, pensámos em `index = 0` e em `offset = 0`. Considerando que temos 6 bits de `offset` e 8 bits de `index`, o primeiro bit de `tag` será o bit 14, e é então aí que podemos construir conflitos. Para testar efetivamente as *duas vias de associatividade*, faz sentido pensar em 3 conflitos (para exceder a capacidade do `set` de `index` 0).

Sendo assim:
1. `0x00000000` - Tag `0x00000`, Index `0x00`, Offset `0x00`
2. `0x00004000` - Tag `0x00001`, Index `0x00`, Offset `0x00`
3. `0x00008000` - Tag `0x00002`, Index `0x00`, Offset `0x00`

A nível de instruções temos:
```c
write(0, (unsigned char *)(&value1)); // 0x00000000
write(16384, (unsigned char *)(&value2)); // 0x00004000
write(32768, (unsigned char *)(&value3)); // 0x00008000

read(0, (unsigned char *)(&value4)); // 0x00000000
read(16384, (unsigned char *)(&value4)); // 0x00004000
read(32768, (unsigned char *)(&value4)); // 0x00008000
read(32768, (unsigned char *)(&value4)); // 0x00008000
```

Sendo que o valor de `value4`, após cada instrução de `read`, bate certo com os valores introduzidos nos `write`'s iniciais, percebemos que a última tarefa está a funcionar como pretendido, relativamente aos valores lidos. Percebemos melhor a utilização de tempo olhando para algumas instruções em específico - a primeira demora `111` unidades de tempo - `100t` ao ler o bloco da memória DRAM, `10t` ao ler esse bloco de L2, e `1t` ao escrever em L1. Já a última apenas demora `1t` uma vez que o endereço `0x00008000` tinha sido utilizado na última instrução, estando então naturalmente em L1. Nas outras instruções, de explicação mais complexa, os tempos também seguem o esperado de caches deste tipo - um *log* destas instruções pode ser lido em `debug.out`.

### Trabalho realizado por

Grupo 6
- 106192 Filipe Oleacu,
- 106459 Fábio Prata,
- 106505 Rodrigo Salgueiro.
