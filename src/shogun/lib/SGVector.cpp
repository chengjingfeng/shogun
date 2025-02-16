/*
 * This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Authors: Soeren Sonnenburg, Viktor Gal, Heiko Strathmann, Fernando Iglesias,
 *          Sanuj Sharma, Pan Deng, Sergey Lisitsyn, Thoralf Klein,
 *          Soumyajit De, Michele Mazzoni, Evgeniy Andreev, Chiyuan Zhang,
 *          Bjoern Esser, Weijie Lin, Khaled Nasr, Koen van de Sande,
 *          Somya Anand
 */

#include <shogun/lib/config.h>
#include <shogun/lib/SGVector.h>
#include <shogun/lib/SGMatrix.h>
#include <shogun/lib/SGSparseVector.h>
#include <shogun/lib/SGReferencedData.h>
#include <shogun/io/File.h>

#include <algorithm>
#include <limits>
#include <shogun/mathematics/Math.h>
#include <shogun/mathematics/lapack.h>
#include <shogun/mathematics/linalg/LinalgNamespace.h>

#define COMPLEX128_ERROR_NOARG(function) \
template <> \
void SGVector<complex128_t>::function() \
{ \
	SG_SERROR("SGVector::%s():: Not supported for complex128_t\n",\
		#function);\
}

#define BOOL_ERROR_ONEARG(function) \
template <> \
void SGVector<bool>::function(bool a) \
{ \
	SG_SERROR("SGVector::%s():: Not supported for bool\n",\
		#function);\
}

#define COMPLEX128_ERROR_ONEARG(function) \
template <> \
void SGVector<complex128_t>::function(complex128_t a) \
{ \
	SG_SERROR("SGVector::%s():: Not supported for complex128_t\n",\
		#function);\
}

#define COMPLEX128_ERROR_TWOARGS(function) \
template <> \
void SGVector<complex128_t>::function(complex128_t a, complex128_t b) \
{ \
	SG_SERROR("SGVector::%s():: Not supported for complex128_t\n",\
		#function);\
}

#define COMPLEX128_ERROR_THREEARGS(function) \
template <> \
void SGVector<complex128_t>::function(complex128_t a, complex128_t b,\
	complex128_t c) \
{ \
	SG_SERROR("SGVector::%s():: Not supported for complex128_t\n",\
		#function);\
}

namespace shogun {

template<class T>
SGVector<T>::SGVector() : SGReferencedData()
{
	init_data();
}

template<class T>
SGVector<T>::SGVector(T* v, index_t len, bool ref_counting)
: SGReferencedData(ref_counting), vector(v), vlen(len), gpu_ptr(NULL)
{
	m_on_gpu.store(false, std::memory_order_release);
}

template<class T>
SGVector<T>::SGVector(T* m, index_t len, index_t offset)
: SGReferencedData(false), vector(m+offset), vlen(len)
{
	m_on_gpu.store(false, std::memory_order_release);
}

template<class T>
SGVector<T>::SGVector(index_t len, bool ref_counting)
: SGReferencedData(ref_counting), vlen(len), gpu_ptr(NULL)
{
	vector=SG_ALIGNED_MALLOC(T, len, alignment::container_alignment);
	std::fill_n(vector, len, 0);
	m_on_gpu.store(false, std::memory_order_release);
}

template <class T>
SGVector<T>::SGVector(SGMatrix<T> matrix)
	: SGReferencedData(matrix), vlen(matrix.num_cols * matrix.num_rows),
	  gpu_ptr(NULL)
{
	ASSERT(!matrix.on_gpu())
	vector = matrix.data();
	m_on_gpu.store(false, std::memory_order_release);
}

template <class T>
SGVector<T>::SGVector(GPUMemoryBase<T>* gpu_vector, index_t len)
	: SGReferencedData(true), vector(NULL), vlen(len),
	  gpu_ptr(std::shared_ptr<GPUMemoryBase<T>>(gpu_vector))
{
	m_on_gpu.store(true, std::memory_order_release);
}

template <class T>
SGVector<T>::SGVector(std::initializer_list<T> il):
	SGVector(il.begin(), il.end())
{
}

template<class T>
SGVector<T>::SGVector(const SGVector &orig) : SGReferencedData(orig)
{
	copy_data(orig);
}

template<class T>
SGVector<T>& SGVector<T>::operator=(const SGVector<T>& other)
{
	if(&other == this)
	return *this;

	unref();
	copy_data(other);
	copy_refcount(other);
	ref();
	return *this;
}

template<class T>
void SGVector<T>::set(SGVector<T> orig)
{
	*this = SGVector<T>(orig);
}

template<class T>
SGVector<T>::~SGVector()
{
	unref();
}

template <class T>
SGVector<T>::SGVector(EigenVectorXt& vec)
: SGReferencedData(false), vector(vec.data()), vlen(vec.size()), gpu_ptr(NULL)
{
	m_on_gpu.store(false, std::memory_order_release);
}

template <class T>
SGVector<T>::SGVector(EigenRowVectorXt& vec)
: SGReferencedData(false), vector(vec.data()), vlen(vec.size()), gpu_ptr(NULL)
{
	m_on_gpu.store(false, std::memory_order_release);
}

template <class T>
SGVector<T>::operator EigenVectorXtMap() const
{
	assert_on_cpu();
	return EigenVectorXtMap(vector, vlen);
}

template <class T>
SGVector<T>::operator EigenRowVectorXtMap() const
{
	assert_on_cpu();
	return EigenRowVectorXtMap(vector, vlen);
}

template<class T>
void SGVector<T>::zero()
{
	assert_on_cpu();
	if (vector && vlen)
		set_const(0);
}

template <>
void SGVector<complex128_t>::zero()
{
	assert_on_cpu();
	if (vector && vlen)
		set_const(complex128_t(0.0));
}

template<class T>
void SGVector<T>::set_const(T const_elem)
{
	assert_on_cpu();
	for (index_t i=0; i<vlen; i++)
		vector[i]=const_elem ;
}

#if HAVE_CATLAS
template<>
void SGVector<float64_t>::set_const(float64_t const_elem)
{
	assert_on_cpu();
	catlas_dset(vlen, const_elem, vector, 1);
}

template<>
void SGVector<float32_t>::set_const(float32_t const_elem)
{
	assert_on_cpu();
	catlas_sset(vlen, const_elem, vector, 1);
}
#endif // HAVE_CATLAS

template<class T>
void SGVector<T>::range_fill(T start)
{
	assert_on_cpu();
	range_fill_vector(vector, vlen, start);
}

COMPLEX128_ERROR_ONEARG(range_fill)

template<class T>
void SGVector<T>::random(T min_value, T max_value)
{
	assert_on_cpu();
	random_vector(vector, vlen, min_value, max_value);
}

COMPLEX128_ERROR_TWOARGS(random)

template <class T>
index_t SGVector<T>::find_position_to_insert(T element)
{
	assert_on_cpu();
	index_t i;
	for (i=0; i<vlen; ++i)
	{
		if (vector[i]>element)
			break;
	}
	return i;
}

template <>
index_t SGVector<complex128_t>::find_position_to_insert(complex128_t element)
{
	SG_SERROR("SGVector::find_position_to_insert():: \
		Not supported for complex128_t\n");
	return index_t(-1);
}

template<class T>
SGVector<T> SGVector<T>::clone() const
{
	if (on_gpu())
		return SGVector<T>(gpu_ptr->clone_vector(gpu_ptr.get(), vlen), vlen);
	else
		return SGVector<T>(clone_vector(vector, vlen), vlen);
}

template<class T>
T* SGVector<T>::clone_vector(const T* vec, int32_t len)
{
	if (!vec || !len)
		return nullptr;

	REQUIRE(len > 0, "Number of elements (%d) has to be positive!\n", len);

	T* result = SG_ALIGNED_MALLOC(T, len, alignment::container_alignment);
	sg_memcpy(result, vec, sizeof(T)*len);
	return result;
}

template<class T>
void SGVector<T>::fill_vector(T* vec, int32_t len, T value)
{
	for (int32_t i=0; i<len; i++)
		vec[i]=value;
}

template<class T>
void SGVector<T>::range_fill_vector(T* vec, int32_t len, T start)
{
	for (int32_t i=0; i<len; i++)
		vec[i]=i+start;
}

template <>
void SGVector<complex128_t>::range_fill_vector(complex128_t* vec,
	int32_t len, complex128_t start)
{
	SG_SERROR("SGVector::range_fill_vector():: \
		Not supported for complex128_t\n");
}

template<class T>
void SGVector<T>::resize_vector(int32_t n)
{
	assert_on_cpu();
	vector=SG_REALLOC(T, vector, vlen, n);

	if (n > vlen)
		memset(&vector[vlen], 0, (n-vlen)*sizeof(T));
	vlen=n;
}

template <class T>
SGVector<T> SGVector<T>::slice(index_t l, index_t h) const
{
	assert_on_cpu();
	ASSERT(l >= 0 && h <= vlen);

	return SGVector<T>(vector, h - l, l);
}

/** addition operator */
template<class T>
SGVector<T> SGVector<T>::operator+ (SGVector<T> x)
{
	assert_on_cpu();
	REQUIRE(x.vector && vector, "Addition possible for only non-null vectors.\n");
	REQUIRE(x.vlen == vlen, "Length of the two vectors to be added should be same. [V(%d) + V(%d)]\n", vlen, x.vlen);

	SGVector<T> result=clone();
	result.add(x);
	return result;
}

template<class T>
void SGVector<T>::add(const SGVector<T> x)
{
	assert_on_cpu();
	REQUIRE(x.vector && vector, "Addition possible for only non-null vectors.\n");
	REQUIRE(x.vlen == vlen, "Length of the two vectors to be added should be same. [V(%d) + V(%d)]\n", vlen, x.vlen);

	for (int32_t i=0; i<vlen; i++)
		vector[i]+=x.vector[i];
}

template<class T>
void SGVector<T>::add(const T x)
{
	assert_on_cpu();
	REQUIRE(vector, "Addition possible for only non-null vectors.\n");
	for (int32_t i=0; i<vlen; i++)
		vector[i]+=x;
}

template<class T>
void SGVector<T>::add(const SGSparseVector<T>& x)
{
	assert_on_cpu();
	if (x.features)
	{
		for (int32_t i=0; i < x.num_feat_entries; i++)
		{
			index_t idx = x.features[i].feat_index;
			REQUIRE(idx < vlen, "Feature index should be less than %d.\n", vlen);
			vector[idx] += x.features[i].entry;
		}
	}
}

template<class T>
void SGVector<T>::display_size() const
{
	assert_on_cpu();
	SG_SPRINT("SGVector '%p' of size: %d\n", vector, vlen)
}

template<class T>
void SGVector<T>::copy_data(const SGReferencedData &orig)
{
	gpu_ptr=std::shared_ptr<GPUMemoryBase<T>>(((SGVector*)(&orig))->gpu_ptr);
	vector=((SGVector*)(&orig))->vector;
	vlen=((SGVector*)(&orig))->vlen;
	m_on_gpu.store(((SGVector*)(&orig))->m_on_gpu.load(
		std::memory_order_acquire), std::memory_order_release);
}

template<class T>
void SGVector<T>::init_data()
{
	vector=NULL;
	vlen=0;
	gpu_ptr=NULL;
	m_on_gpu.store(false, std::memory_order_release);
}

template<class T>
void SGVector<T>::free_data()
{
	SG_FREE(vector);
	vector=NULL;
	vlen=0;
	gpu_ptr=NULL;
}

template <class T>
bool SGVector<T>::equals(const SGVector<T>& other) const
{
	assert_on_cpu();
	if (other.vlen!=vlen)
		return false;

	if (vector == other.vector)
		return true;

	for (index_t i=0; i<vlen; ++i)
	{
		if (other.vector[i]!=vector[i])
			return false;
	}

	return true;
}

#ifndef REAL_EQUALS
#define REAL_EQUALS(real_t)                                                    \
	template <>                                                                \
	bool SGVector<real_t>::equals(const SGVector<real_t>& other) const         \
	{                                                                          \
		assert_on_cpu();                                                       \
		if (other.vlen != vlen)                                                \
			return false;                                                      \
                                                                               \
		if (vector == other.vector)                                            \
			return true;                                                       \
                                                                               \
		for (index_t i = 0; i < vlen; ++i)                                     \
		{                                                                      \
			if (!CMath::fequals(                                               \
			        vector[i], other.vector[i],                                \
			        std::numeric_limits<real_t>::epsilon()))                   \
				return false;                                                  \
		}                                                                      \
		return true;                                                           \
	}
REAL_EQUALS(float32_t)
REAL_EQUALS(float64_t)
REAL_EQUALS(floatmax_t)
#undef REAL_EQUALS
#endif // REAL_EQUALS

template<class T>
std::string SGVector<T>::to_string() const
{
	return to_string(vector, vlen);
}

template<class T>
std::string SGVector<T>::to_string(const T* vector, index_t n)
{
	std::stringstream ss;
	ss << "[";
	if (n > 0)
	{
		const T* begin = vector;
		const T* end = vector+n;
		ss << std::boolalpha << *begin++;
		std::for_each(begin, end, [&ss](auto _v) { ss << ", " << _v; });
	}
	ss << "]";
	return ss.str();
}

template<class T>
void SGVector<T>::display_vector(const char* name,
		const char* prefix) const
{
	display_vector(vector, vlen, name, prefix);
}

template <class T>
void SGVector<T>::display_vector(const SGVector<T> vector, const char* name,
		const char* prefix)
{
	vector.display_vector(prefix);
}

template <class T>
void SGVector<T>::display_vector(const T* vector, int32_t n, const char* name,
		const char* prefix)
{
	REQUIRE(n>=0, "Vector size can not be negative.\n");
	SG_SPRINT("%s%s=%s\n", prefix, name, to_string(vector, n).c_str())
}

template <class T>
void SGVector<T>::vec1_plus_scalar_times_vec2(T* vec1,
		const T scalar, const T* vec2, int32_t n)
{
	for (int32_t i=0; i<n; i++)
		vec1[i]+=scalar*vec2[i];
}

template <>
void SGVector<float64_t>::vec1_plus_scalar_times_vec2(float64_t* vec1,
		float64_t scalar, const float64_t* vec2, int32_t n)
{
#ifdef HAVE_LAPACK
	int32_t skip=1;
	cblas_daxpy(n, scalar, vec2, skip, vec1, skip);
#else
	for (int32_t i=0; i<n; i++)
		vec1[i]+=scalar*vec2[i];
#endif
}

template <>
void SGVector<float32_t>::vec1_plus_scalar_times_vec2(float32_t* vec1,
		float32_t scalar, const float32_t* vec2, int32_t n)
{
#ifdef HAVE_LAPACK
	int32_t skip=1;
	cblas_saxpy(n, scalar, vec2, skip, vec1, skip);
#else
	for (int32_t i=0; i<n; i++)
		vec1[i]+=scalar*vec2[i];
#endif
}

template <class T>
	void SGVector<T>::random_vector(T* vec, int32_t len, T min_value, T max_value)
	{
		for (int32_t i=0; i<len; i++)
			vec[i]=CMath::random(min_value, max_value);
	}

template <>
void SGVector<complex128_t>::random_vector(complex128_t* vec, int32_t len,
	complex128_t min_value, complex128_t max_value)
{
	SG_SNOTIMPLEMENTED
}

template <>
bool SGVector<bool>::twonorm(const bool* x, int32_t len)
{
	SG_SNOTIMPLEMENTED
	return false;
}

template <>
char SGVector<char>::twonorm(const char* x, int32_t len)
{
	SG_SNOTIMPLEMENTED
	return '\0';
}

template <>
int8_t SGVector<int8_t>::twonorm(const int8_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
uint8_t SGVector<uint8_t>::twonorm(const uint8_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
int16_t SGVector<int16_t>::twonorm(const int16_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
uint16_t SGVector<uint16_t>::twonorm(const uint16_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
int32_t SGVector<int32_t>::twonorm(const int32_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
uint32_t SGVector<uint32_t>::twonorm(const uint32_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
int64_t SGVector<int64_t>::twonorm(const int64_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
uint64_t SGVector<uint64_t>::twonorm(const uint64_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
float32_t SGVector<float32_t>::twonorm(const float32_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
float64_t SGVector<float64_t>::twonorm(const float64_t* v, int32_t n)
{
#ifdef HAVE_LAPACK
	return cblas_dnrm2(n, v, 1);
#else
	SGVector<float64_t> wrapper(const_cast<float64_t*>(v), n, false);
	return std::sqrt(linalg::dot(wrapper, wrapper));
#endif
}

template <>
floatmax_t SGVector<floatmax_t>::twonorm(const floatmax_t* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <>
complex128_t SGVector<complex128_t>::twonorm(const complex128_t* x, int32_t len)
{
	complex128_t result(0.0);
	for (int32_t i=0; i<len; i++)
		result+=x[i]*x[i];

	return std::sqrt(result);
}

template <class T>
float64_t SGVector<T>::onenorm(T* x, int32_t len)
{
	float64_t result=0;
	for (int32_t i=0;i<len; ++i)
		result+=CMath::abs(x[i]);

	return result;
}

/// || x ||_q^q
template <class T>
T SGVector<T>::qsq(T* x, int32_t len, float64_t q)
{
	float64_t result=0;
	for (int32_t i=0; i<len; i++)
		result+=CMath::pow(fabs(x[i]), q);

	return result;
}

template <>
complex128_t SGVector<complex128_t>::qsq(complex128_t* x, int32_t len, float64_t q)
{
	SG_SNOTIMPLEMENTED
	return complex128_t(0.0);
}

/// || x ||_q
template <class T>
T SGVector<T>::qnorm(T* x, int32_t len, float64_t q)
{
	REQUIRE(q!=0, "Q should be non-zero for calculating qnorm\n");
	return CMath::pow((float64_t) qsq(x, len, q), 1.0/q);
}

template <>
complex128_t SGVector<complex128_t>::qnorm(complex128_t* x, int32_t len, float64_t q)
{
	SG_SNOTIMPLEMENTED
	return complex128_t(0.0);
}

/// return sum(abs(vec))
template <class T>
T SGVector<T>::sum_abs(T* vec, int32_t len)
{
	T result=0;
	for (int32_t i=0; i<len; i++)
		result+=CMath::abs(vec[i]);

	return result;
}

#if HAVE_LAPACK
template <>
float64_t SGVector<float64_t>::sum_abs(float64_t* vec, int32_t len)
{
	float64_t result=0;
	result = cblas_dasum(len, vec, 1);
	return result;
}

template <>
float32_t SGVector<float32_t>::sum_abs(float32_t* vec, int32_t len)
{
	float32_t result=0;
	result = cblas_sasum(len, vec, 1);
	return result;
}
#endif

template <class T>
int32_t SGVector<T>::unique(T* output, int32_t size)
{
	std::sort(output, output+size);
	auto last = std::unique(output, output+size);
	return std::distance(output, last);
}

template <>
int32_t SGVector<complex128_t>::unique(complex128_t* output, int32_t size)
{
	int32_t j=0;
	SG_SERROR("SGVector::unique():: Not supported for complex128_t\n");
	return j;
}

template <class T>
SGVector<T> SGVector<T>::unique()
{
	SGVector<T> result = clone();
	auto new_size = unique(result.data(), result.size());
	result.resize_vector(new_size);
	return result;
}

template <>
SGVector<complex128_t> SGVector<complex128_t>::unique()
{
	SG_SNOTIMPLEMENTED
	return SGVector<complex128_t>();
}

template <class T>
SGVector<index_t> SGVector<T>::find(T elem)
{
	assert_on_cpu();
	SGVector<index_t> idx(vlen);
	index_t k=0;

	for (index_t i=0; i < vlen; ++i)
		if (vector[i] == elem)
			idx[k++] = i;
	idx.vlen = k;
	return idx;
}

template<class T>
void SGVector<T>::scale_vector(T alpha, T* vec, int32_t len)
{
	for (int32_t i=0; i<len; i++)
		vec[i]*=alpha;
}

#ifdef HAVE_LAPACK
template<>
void SGVector<float64_t>::scale_vector(float64_t alpha, float64_t* vec, int32_t len)
{
	cblas_dscal(len, alpha, vec, 1);
}

template<>
void SGVector<float32_t>::scale_vector(float32_t alpha, float32_t* vec, int32_t len)
{
	cblas_sscal(len, alpha, vec, 1);
}
#endif

template<class T>
void SGVector<T>::scale(T alpha)
{
	scale_vector(alpha, vector, vlen);
}

template<class T> void SGVector<T>::load(CFile* loader)
{
	REQUIRE(loader, "No file provided.\n");
	unref();

	SG_SET_LOCALE_C;
	SGVector<T> vec;
	loader->get_vector(vec.vector, vec.vlen);
	vec.gpu_ptr = nullptr;
	copy_data(vec);
	copy_refcount(vec);
	ref();
	SG_RESET_LOCALE;
}

template<>
void SGVector<complex128_t>::load(CFile* loader)
{
	SG_SERROR("SGVector::load():: Not supported for complex128_t\n");
}

template<class T> void SGVector<T>::save(CFile* saver)
{
	REQUIRE(saver, "Requires a valid 'c FILE pointer'\n");

	assert_on_cpu();
	SG_SET_LOCALE_C;
	saver->set_vector(vector, vlen);
	SG_RESET_LOCALE;
}

template<>
void SGVector<complex128_t>::save(CFile* saver)
{
	SG_SERROR("SGVector::save():: Not supported for complex128_t\n");
}

template <class T> SGVector<float64_t> SGVector<T>::get_real()
{
	assert_on_cpu();
	SGVector<float64_t> real(vlen);
	for (int32_t i=0; i<vlen; i++)
		real[i]=CMath::real(vector[i]);
	return real;
}

template <class T> SGVector<float64_t> SGVector<T>::get_imag()
{
	assert_on_cpu();
	SGVector<float64_t> imag(vlen);
	for (int32_t i=0; i<vlen; i++)
		imag[i]=CMath::imag(vector[i]);
	return imag;
}

template <class T>
SGMatrix<T> SGVector<T>::convert_to_matrix(SGVector<T> vector,
	index_t nrows, index_t ncols, bool fortran_order)
{
	if (nrows*ncols>vector.size())
		SG_SERROR("SGVector::convert_to_matrix():: Dimensions mismatch\n");

	T* data=NULL;
	SGVector<T>::convert_to_matrix(data, nrows, ncols, vector.vector, vector.vlen, fortran_order);

	SGMatrix<T> matrix=SGMatrix<T>(data, nrows, ncols);
	return matrix;
}

template <class T>
void SGVector<T>::convert_to_matrix(T*& matrix, index_t nrows, index_t ncols, const T* vector, int32_t vlen, bool fortran_order)
{
	if (nrows*ncols>vlen)
		SG_SERROR("SGVector::convert_to_matrix():: Dimensions mismatch\n");

	if (matrix!=NULL)
		SG_FREE(matrix);
	matrix=SG_ALIGNED_MALLOC(T, nrows*ncols, alignment::container_alignment);

	if (fortran_order)
	{
		for (index_t i=0; i<ncols*nrows; i++)
			matrix[i]=vector[i];
	}
	else
	{
		for (index_t i=0; i<nrows; i++)
		{
			for (index_t j=0; j<ncols; j++)
				matrix[i+j*nrows]=vector[j+i*ncols];
		}
	}
}

#define UNDEFINED(function, type)	\
template <>	\
SGVector<float64_t> SGVector<type>::function()	\
{	\
	SG_SERROR("SGVector::%s():: Not supported for %s\n",	\
		#function, #type);	\
	SGVector<float64_t> ret(vlen);	\
	return ret;	\
}

UNDEFINED(get_real, bool)
UNDEFINED(get_real, char)
UNDEFINED(get_real, int8_t)
UNDEFINED(get_real, uint8_t)
UNDEFINED(get_real, int16_t)
UNDEFINED(get_real, uint16_t)
UNDEFINED(get_real, int32_t)
UNDEFINED(get_real, uint32_t)
UNDEFINED(get_real, int64_t)
UNDEFINED(get_real, uint64_t)
UNDEFINED(get_real, float32_t)
UNDEFINED(get_real, float64_t)
UNDEFINED(get_real, floatmax_t)
UNDEFINED(get_imag, bool)
UNDEFINED(get_imag, char)
UNDEFINED(get_imag, int8_t)
UNDEFINED(get_imag, uint8_t)
UNDEFINED(get_imag, int16_t)
UNDEFINED(get_imag, uint16_t)
UNDEFINED(get_imag, int32_t)
UNDEFINED(get_imag, uint32_t)
UNDEFINED(get_imag, int64_t)
UNDEFINED(get_imag, uint64_t)
UNDEFINED(get_imag, float32_t)
UNDEFINED(get_imag, float64_t)
UNDEFINED(get_imag, floatmax_t)
#undef UNDEFINED

template class SGVector<bool>;
template class SGVector<char>;
template class SGVector<int8_t>;
template class SGVector<uint8_t>;
template class SGVector<int16_t>;
template class SGVector<uint16_t>;
template class SGVector<int32_t>;
template class SGVector<uint32_t>;
template class SGVector<int64_t>;
template class SGVector<uint64_t>;
template class SGVector<float32_t>;
template class SGVector<float64_t>;
template class SGVector<floatmax_t>;
template class SGVector<complex128_t>;
}

#undef COMPLEX128_ERROR_NOARG
#undef COMPLEX128_ERROR_ONEARG
#undef COMPLEX128_ERROR_TWOARGS
#undef COMPLEX128_ERROR_THREEARGS
