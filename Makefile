CC ?= gcc
CPP ?= cpp
AR ?= ar
OBLIVCC = $(OBLIVC_PATH)/bin/oblivcc
OBLIVCH = $(OBLIVC_PATH)/src/ext/oblivc
OBLIVCA = $(OBLIVC_PATH)/_build/libobliv.a
CFLAGS+= -O3 -march=native -I/usr/include -I . -I $(SRC_PATH) -std=c99 -fopenmp
LDFLAGS += -lm -lgomp -lgcrypt

SRC_PATH=src/
LIB_OUT_PATH=build/lib/
ACKLIB = $(LIB_OUT_PATH)/liback.a
DEPS=ackutil.o endian.oo shuffle.oo waksman.o
SQRT_ORAM_DEPS=decoder.oo sqrtoram.oo
CKT_ORAM_DEPS=block.oo circuit_oram.oo linear_scan_oram.oo nonrecursive_oram.oo utils.oo
FSSL_ORAM_DEPS=fss.oo fss.o fss_cprg.oo fss_cprg.o floram_util.oo floram_util.o scanrom.oo scanrom.o floram.oo\
		aes_gladman/aeskey.o aes_gladman/aestab.o aes_gladman/aescrypt.o
ORAM_DEPS = $(SQRT_ORAM_DEPS:%=oram_sqrt/%)  $(CKT_ORAM_DEPS:%=oram_ckt/%) $(FSSL_ORAM_DEPS:%=oram_fssl/%) oram.oo
OBJS=$(DEPS) $(ORAM_DEPS) obig.oo ochacha.oo ograph.oo omatch.oo oqueue.oo\
		osalsa.oo oscrypt.oo osearch.oo osha256.oo osha512.oo osort.oo oaes.oo

TEST_PATH=tests/
TEST_OUT_PATH=build/tests/
TEST_DEPS=test_main.o
TEST_BINS = test_obig test_osha256 test_osha512 test_osalsa test_ochacha test_oaes\
		test_oqueue test_oram test_oscrypt test_ograph test_omatch test_osearch\
		bench_oram_write bench_oram_read bench_oram_init bench_oscrypt bench_bfs bench_bs\
		bench_gs bench_rp bench_oaes bench_oqueue bench_waksman

default: $(ACKLIB) tests

tests: $(TEST_BINS:%=$(TEST_OUT_PATH)/%)

$(TEST_BINS:%=$(TEST_OUT_PATH)/%): $(TEST_OUT_PATH)/%: $(TEST_PATH)/%.oo $(TEST_DEPS:%=$(TEST_PATH)/%) $(ACKLIB)
	mkdir -p $(TEST_OUT_PATH)
	$(OBLIVCC) -o $@ $(OBLIVCA) $^ $(LDFLAGS)

$(ACKLIB): $(OBJS:%=$(SRC_PATH)/%)
	mkdir -p $(LIB_OUT_PATH)
	$(AR) rcs $@ $^

-include $($(patsubst %.oo,%.od,$(OBJS:.o=.d)):%=$(SRC_PATH)/%) $(TEST_BINS:%=$(TEST_PATH)/%.od)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o -I $(OBLIVCH)
	$(CPP) -MM $(CFLAGS) $*.c -I $(OBLIVCH) > $*.d

%.fssl.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_FSSL $*.oc -o $*.sqrt.oo
	$(CPP) -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_FSSL $*.oc -MT $*.sqrt.oo > $*.sqrt.od

%.sqrt.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_SQRT $*.oc -o $*.sqrt.oo
	$(CPP) -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_SQRT $*.oc -MT $*.sqrt.oo > $*.sqrt.od

%.circuit.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_CIRCUIT $*.oc -o $*.circuit.oo
	$(CPP) -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_CIRCUIT $*.oc -MT $*.circuit.oo > $*.circuit.od

%.linear.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_LINEAR $*.oc -o $*.linear.oo
	$(CPP) -MM $(CFLAGS) -DORAM_OVERRIDE=ORAM_TYPE_LINEAR $*.oc -MT $*.linear.oo > $*.linear.od

%.oo: %.oc
	$(OBLIVCC) -c $(CFLAGS) $*.oc -o $*.oo
	$(CPP) -MM $(CFLAGS) $*.oc -MT $*.oo > $*.od

clean:
	rm -f $(OBJS:%=$(SRC_PATH)/%) $(patsubst %.oo,$(SRC_PATH)/%.od,$(patsubst %.o,$(SRC_PATH)/%.d,$(OBJS))) $(ACKLIB)
	rm -f $(TEST_BINS:%=$(TEST_OUT_PATH)/%) $(TEST_DEPS:%=$(TEST_PATH)/%) $($(pasubst %.oo, %.od, $(TEST_DEPS)):%=$(TEST_PATH)/%) $(TEST_BINS:%=$(TEST_PATH)/%.oo) $(TEST_BINS:%=$(TEST_PATH)/%.od)
