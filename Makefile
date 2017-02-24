OBLIVCC = $(OBLIVC_PATH)/bin/oblivcc
OBLIVCH = $(OBLIVC_PATH)/src/ext/oblivc
OBLIVCA = $(OBLIVC_PATH)/_build/libobliv.a
CFLAGS+= -O3 -march=native -I/usr/include -I . -I $(SRC_PATH) -std=c99 -fopenmp

SRC_PATH=src/
LIB_OUT_PATH=build/lib/
ACKLIB = $(LIB_OUT_PATH)/liback.a
DEPS=ackutil.o endian.oo shuffle.oo waksman.o
SQRT_ORAM_DEPS=decoder.oo sqrtoram.oo
CKT_ORAM_DEPS=block.oo circuit_oram.oo linear_scan_oram.oo nonrecursive_oram.oo utils.oo
FSSL_ORAM_DEPS=bitpropagate.oo bitpropagate.o bitpropagate_cprg.oo bitpropagate_cprg.o floram_util.oo floram_util.o scanrom.oo scanrom.o floram.oo
ORAM_DEPS = $(SQRT_ORAM_DEPS:%=oram_sqrt/%)  $(CKT_ORAM_DEPS:%=oram_ckt/%) $(FSSL_ORAM_DEPS:%=oram_fssl/%) oram.oo
OBJS=$(DEPS) $(ORAM_DEPS) obig.oo ochacha.oo ograph.oo omatch.oo oqueue.oo\
		osalsa.oo oscrypt.oo osearch.oo osha256.oo osha512.oo osort.oo oaes.oo

TEST_PATH=tests/
TEST_OUT_PATH=build/tests/
TEST_DEPS=test_main.o
TEST_BINS = test_obig test_osha256 test_osha512 test_osalsa test_ochacha test_oaes\
		test_oqueue test_oram test_oscrypt test_ograph test_omatch test_osearch\
		bench_oram bench_oram_init bench_oscrypt bench_bfs bench_bst bench_gs bench_rp\
		bench_oaes test_floram bench_floram_init bench_oram_read

default: $(ACKLIB) tests

tests: $(TEST_BINS:%=$(TEST_OUT_PATH)/%)

$(TEST_BINS:%=$(TEST_OUT_PATH)/%): $(TEST_OUT_PATH)/%: $(TEST_PATH)/%.oo $(TEST_DEPS:%=$(TEST_PATH)/%) $(ACKLIB)
	mkdir -p $(TEST_OUT_PATH)
	$(OBLIVCC) -o $@ $(OBLIVCA) $^ -lm -lssl -lcrypto -lgomp

$(ACKLIB): $(OBJS:%=$(SRC_PATH)/%)
	mkdir -p $(LIB_OUT_PATH)
	ar rcs $@ $^

-include $(patsubst %.oo,%.od,$(OBJS:.o=.d))

%.o: %.c
	gcc -c $(CFLAGS) $*.c -o $*.o -I $(OBLIVCH)
	cpp -MM $(CFLAGS) $*.c -I $(OBLIVCH) > $*.d

%.fssl.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_FSSL $*.oc -o $*.sqrt.oo
	cpp -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_FSSL $*.oc -MT $*.sqrt.oo > $*.sqrt.od

%.sqrt.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_SQRT $*.oc -o $*.sqrt.oo
	cpp -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_SQRT $*.oc -MT $*.sqrt.oo > $*.sqrt.od

%.circuit.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_CIRCUIT $*.oc -o $*.circuit.oo
	cpp -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_CIRCUIT $*.oc -MT $*.circuit.oo > $*.circuit.od

%.linear.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_LINEAR $*.oc -o $*.linear.oo
	cpp -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_LINEAR $*.oc -MT $*.linear.oo > $*.linear.od

%.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) $*.oc -o $*.oo
	cpp -MM $(CFLAGS) $*.oc -MT $*.oo > $*.od

clean:
	rm -f $(OBJS:%=$(SRC_PATH)/%) $(patsubst %.oo,$(SRC_PATH)/%.od,$(patsubst %.o,$(SRC_PATH)/%.d,$(OBJS))) $(ACKLIB)
	rm -f $(TEST_BINS:%=$(TEST_OUT_PATH)/%) $(TEST_DEPS:%=$(TEST_PATH)/%) $($(pasubst %.oo, %.od, $(TEST_DEPS)):%=$(TEST_PATH)/%) $(TEST_BINS:%=$(TEST_PATH)/%.oo) $(TEST_BINS:%=$(TEST_PATH)/%.od)
