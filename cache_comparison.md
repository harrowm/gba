# ARM CPU Instruction Cache Size Comparison

## 1024-Entry Cache
```
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       235660084 IPS
SUB R1, R1, R2       355694672 IPS
MOV R1, R2           389787566 IPS
ORR R1, R1, R2       369412634 IPS
AND R1, R1, R2       399536538 IPS
CMP R1, R2           357347055 IPS

=== ARM Instruction Cache Statistics ===
Cache hits: 59994060
Cache misses: 6000
Cache invalidations: 0
Cache hit rate: 99.99%
```

## 1024-Entry Cache Performance (Multiple Runs)
```
Run 1:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       371857802 IPS
SUB R1, R1, R2       373482726 IPS
MOV R1, R2           371816323 IPS
ORR R1, R1, R2       387011881 IPS
AND R1, R1, R2       378917055 IPS
CMP R1, R2           353506787 IPS

Run 2:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       387551835 IPS
SUB R1, R1, R2       367309458 IPS
MOV R1, R2           376180266 IPS
ORR R1, R1, R2       382980353 IPS
AND R1, R1, R2       379319501 IPS
CMP R1, R2           347741419 IPS

Run 3:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       356099993 IPS
SUB R1, R1, R2       375361285 IPS
MOV R1, R2           370878611 IPS
ORR R1, R1, R2       397456280 IPS
AND R1, R1, R2       366689890 IPS
CMP R1, R2           346548378 IPS

Run 4:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       354572209 IPS
SUB R1, R1, R2       376931775 IPS
MOV R1, R2           377386973 IPS
ORR R1, R1, R2       368432687 IPS
AND R1, R1, R2       389650873 IPS
CMP R1, R2           356938892 IPS

Run 5:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       356188780 IPS
SUB R1, R1, R2       371802499 IPS
MOV R1, R2           381970970 IPS
ORR R1, R1, R2       379996960 IPS
AND R1, R1, R2       367134151 IPS
CMP R1, R2           357845768 IPS

```

Average IPS for 1024-entry cache: 370985412

## 2048-Entry Cache
```
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       247831475 IPS
SUB R1, R1, R2       343371219 IPS
MOV R1, R2           363121391 IPS
ORR R1, R1, R2       363927506 IPS
AND R1, R1, R2       355935220 IPS
CMP R1, R2           343135573 IPS

=== ARM Instruction Cache Statistics ===
Cache hits: 59994060
Cache misses: 6000
Cache invalidations: 0
Cache hit rate: 99.99%
```

## 2048-Entry Cache Performance (Multiple Runs)
```
Run 1:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       349381595 IPS
SUB R1, R1, R2       350287236 IPS
MOV R1, R2           386503305 IPS
ORR R1, R1, R2       373999551 IPS
AND R1, R1, R2       379968083 IPS
CMP R1, R2           353020087 IPS

Run 2:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       342665250 IPS
SUB R1, R1, R2       347862386 IPS
MOV R1, R2           375544540 IPS
ORR R1, R1, R2       364763815 IPS
AND R1, R1, R2       378243437 IPS
CMP R1, R2           342161089 IPS

Run 3:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       362187613 IPS
SUB R1, R1, R2       374349568 IPS
MOV R1, R2           394181876 IPS
ORR R1, R1, R2       397440483 IPS
AND R1, R1, R2       389271673 IPS
CMP R1, R2           361964745 IPS

Run 4:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       381985561 IPS
SUB R1, R1, R2       390076455 IPS
MOV R1, R2           373622268 IPS
ORR R1, R1, R2       368785957 IPS
AND R1, R1, R2       375516335 IPS
CMP R1, R2           350250429 IPS

Run 5:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       361794501 IPS
SUB R1, R1, R2       364843664 IPS
MOV R1, R2           365296804 IPS
ORR R1, R1, R2       364245647 IPS
AND R1, R1, R2       392649599 IPS
CMP R1, R2           339477883 IPS

```

Average IPS for 2048-entry cache: 368411381

## 4096-Entry Cache
```
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       188068908 IPS
SUB R1, R1, R2       345375423 IPS
MOV R1, R2           384393619 IPS
ORR R1, R1, R2       381359164 IPS
AND R1, R1, R2       381024957 IPS
CMP R1, R2           351469141 IPS

=== ARM Instruction Cache Statistics ===
Cache hits: 59994060
Cache misses: 6000
Cache invalidations: 0
Cache hit rate: 99.99%
```

## 4096-Entry Cache Performance (Multiple Runs)
```
Run 1:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       346308353 IPS
SUB R1, R1, R2       337952011 IPS
MOV R1, R2           350594257 IPS
ORR R1, R1, R2       357270454 IPS
AND R1, R1, R2       347886589 IPS
CMP R1, R2           338615739 IPS

Run 2:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       366448019 IPS
SUB R1, R1, R2       375502234 IPS
MOV R1, R2           361010830 IPS
ORR R1, R1, R2       357359826 IPS
AND R1, R1, R2       370919881 IPS
CMP R1, R2           342067456 IPS

Run 3:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       348869662 IPS
SUB R1, R1, R2       346644481 IPS
MOV R1, R2           335019599 IPS
ORR R1, R1, R2       345661943 IPS
AND R1, R1, R2       346356331 IPS
CMP R1, R2           339881721 IPS

Run 4:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       355063201 IPS
SUB R1, R1, R2       357641000 IPS
MOV R1, R2           363306085 IPS
ORR R1, R1, R2       345506686 IPS
AND R1, R1, R2       343926262 IPS
CMP R1, R2           338638673 IPS

Run 5:
=== ALU Operation Focused Benchmark ===

ADD R1, R1, R2       346452328 IPS
SUB R1, R1, R2       342079157 IPS
MOV R1, R2           343300491 IPS
ORR R1, R1, R2       344103782 IPS
AND R1, R1, R2       353894610 IPS
CMP R1, R2           337006706 IPS

```

Average IPS for 4096-entry cache: 349509612

## Summary Comparison

| Cache Size | Average IPS | Cache Hit Rate | Relative Performance |
|------------|-------------|----------------|----------------------|
| 1024 | 370985412 | 99.99% | +0% |
| 2048 | 368411381 | 99.99% | +-.69% |
| 4096 | 349509612 | 99.99% | +-5.78% |

## Performance Analysis

- Performance gain from 1024 to 2048 entries: +-.69%
- Performance gain from 2048 to 4096 entries: +-5.13%

**Recommendation**: The 1024-entry cache provides adequate performance. Increasing cache size shows minimal gains.
