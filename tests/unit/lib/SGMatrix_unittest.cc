#include <gtest/gtest.h>

#include <shogun/lib/SGMatrix.h>
#include <shogun/lib/SGVector.h>
#include <shogun/mathematics/Math.h>
#include <shogun/mathematics/linalg/LinalgNamespace.h>

#include <shogun/mathematics/eigen3.h>

#ifdef HAVE_VIENNACL
#include <shogun/mathematics/linalg/LinalgBackendViennaCL.h>
#endif

using namespace shogun;

TEST(SGMatrixTest,ctor_zero_const)
{
	SGMatrix<float64_t> a(10, 5);
	EXPECT_EQ(a.num_rows, 10);
	EXPECT_EQ(a.num_cols, 5);

	a.zero();
	for (int i=0; i < 10; ++i)
	{
		for (int j=0; j < 5; ++j)
		EXPECT_EQ(0, a(i,j));
	}

	a.set_const(3.3);
	for (int i=0; i < 10; ++i)
	{
		for (int j=0; j < 5; ++j)
		EXPECT_EQ(3.3, a(i,j));
	}
}

TEST(SGMatrixTest, SGVector_ctor)
{
	SGVector<float64_t> v(5);
	for (index_t i=0; i<5; ++i)
		v[i]=i;

	EXPECT_EQ(v.ref_count(), 1);
	{
		SGMatrix<float64_t> m(v);
		EXPECT_EQ(v.ref_count(), 2);
		EXPECT_EQ(m.num_rows, 5);
		EXPECT_EQ(m.num_cols, 1);

		for (index_t j=0; j<m.num_cols; ++j)
		{
			for (index_t i=0; i<m.num_rows; ++i)
				EXPECT_NEAR(m(i, j), v[j*m.num_rows+i], 1e-15);
		}
	}
	EXPECT_EQ(v.ref_count(), 1);
}

TEST(SGMatrixTest, SGVector_ctor_row_col_specified)
{
	SGVector<float64_t> v(6);
	for (index_t i=0; i<6; ++i)
		v[i]=i;

	EXPECT_EQ(v.ref_count(), 1);
	{
		SGMatrix<float64_t> m(v, 3, 2);
		EXPECT_EQ(v.ref_count(), 2);
		EXPECT_EQ(m.num_rows, 3);
		EXPECT_EQ(m.num_cols, 2);

		for (index_t j=0; j<m.num_cols; ++j)
		{
			for (index_t i=0; i<m.num_rows; ++i)
				EXPECT_NEAR(m(i, j), v[j*m.num_rows+i], 1e-15);
		}
	}
	EXPECT_EQ(v.ref_count(), 1);
}

TEST(SGMatrixTest, SGVector_ctor_no_refcount)
{
	float64_t *vec=SG_MALLOC(float64_t, 6);
	SGVector<float64_t> v(vec, 6, false);
	for (index_t i=0; i<6; ++i)
		v[i]=i;

	EXPECT_EQ(v.ref_count(), -1);
	{
		SGMatrix<float64_t> m(v, 3, 2);
		EXPECT_EQ(v.ref_count(), -1);
		EXPECT_EQ(m.num_rows, 3);
		EXPECT_EQ(m.num_cols, 2);

		for (index_t j=0; j<m.num_cols; ++j)
		{
			for (index_t i=0; i<m.num_rows; ++i)
				EXPECT_NEAR(m(i, j), v[j*m.num_rows+i], 1e-15);
		}
	}
	EXPECT_EQ(v.ref_count(), -1);
	SG_FREE(vec);
}

TEST(SGMatrixTest,setget)
{
	SGMatrix<index_t> v(3,2);
	v(0,0)=1;
	v(0,1)=2;
	v(1,0)=3;
	v(1,1)=4;
	v(2,0)=5;
	v(2,1)=6;

	EXPECT_EQ(v(0,0), 1);
	EXPECT_EQ(v(0,1), 2);
	EXPECT_EQ(v(1,0), 3);
	EXPECT_EQ(v(1,1), 4);
	EXPECT_EQ(v(2,0), 5);
	EXPECT_EQ(v(2,1), 6);
}

TEST(SGMatrixTest,equals_equal)
{
	SGMatrix<float64_t> a(3,2);
	SGMatrix<float64_t> b(3,2);
	a(0,0)=1;
	a(0,1)=2;
	a(1,0)=3;
	a(1,1)=4;
	a(2,0)=5;
	a(2,1)=6;

	b(0,0)=1;
	b(0,1)=2;
	b(1,0)=3;
	b(1,1)=4;
	b(2,0)=5;
	b(2,1)=6;

	EXPECT_TRUE(a.equals(b));
}

TEST(SGMatrixTest,equals_different)
{
	SGMatrix<float64_t> a(3,2);
	SGMatrix<float64_t> b(3,2);
	a(0,0)=1;
	a(0,1)=2;
	a(1,0)=3;
	a(1,1)=4;
	a(2,0)=5;
	a(2,1)=6;

	b(0,0)=1;
	b(0,1)=2;
	b(1,0)=3;
	b(1,1)=4;
	b(2,0)=5;
	b(2,1)=7;

	EXPECT_FALSE(a.equals(b));
}

TEST(SGMatrixTest,equals_different_size)
{
	SGMatrix<float64_t> a(3,2);
	SGMatrix<float64_t> b(2,2);
	a.zero();
	b.zero();

	EXPECT_FALSE(a.equals(b));
}

TEST(SGMatrixTest, equals_both_empty)
{
	SGMatrix<float64_t> a;
	SGMatrix<float64_t> b;

	EXPECT_TRUE(a.equals(b));
	EXPECT_TRUE(b.equals(a));
}

#ifdef HAVE_VIENNACL
TEST(SGMatrixTest,pointer_equal_equal)
{
	sg_linalg->set_gpu_backend(new LinalgBackendViennaCL());

	SGMatrix<float64_t> a(3,2), a_gpu;
	a.zero();
	linalg::to_gpu(a, a_gpu);
	SGMatrix<float64_t> b_gpu(a_gpu);

	EXPECT_TRUE(a_gpu == b_gpu);
}

TEST(SGMatrixTest,pointer_equal_different)
{
	sg_linalg->set_gpu_backend(new LinalgBackendViennaCL());

	SGMatrix<float64_t> a(3,2), a_gpu;
	a.zero();
	linalg::to_gpu(a, a_gpu);

	SGMatrix<float64_t> b(3,2), b_gpu;
	b.zero();
	linalg::to_gpu(b, b_gpu);

	EXPECT_FALSE(a_gpu == b_gpu);
}
#endif

TEST(SGMatrixTest,get_diagonal_vector_square_matrix)
{
	SGMatrix<int32_t> mat(3, 3);

	mat(0,0)=8;
	mat(0,1)=1;
	mat(0,2)=6;
	mat(1,0)=3;
	mat(1,1)=5;
	mat(1,2)=7;
	mat(2,0)=4;
	mat(2,1)=9;
	mat(2,2)=2;

	SGVector<int32_t> diag=mat.get_diagonal_vector();

	EXPECT_EQ(diag[0], 8);
	EXPECT_EQ(diag[1], 5);
	EXPECT_EQ(diag[2], 2);
}

TEST(SGMatrixTest,get_diagonal_vector_rectangular_matrix)
{
	SGMatrix<int32_t> mat(3, 2);

	mat(0,0)=8;
	mat(0,1)=1;
	mat(1,0)=3;
	mat(1,1)=5;
	mat(2,0)=4;
	mat(2,1)=9;

	SGVector<int32_t> diag=mat.get_diagonal_vector();

	EXPECT_EQ(diag[0], 8);
	EXPECT_EQ(diag[1], 5);
}

TEST(SGMatrixTest,is_symmetric_float32_false_old_plus_eps)
{
	const index_t size=2;
	SGMatrix<float32_t> mat(size, size);
	CMath::init_random(100);

	// create a symmetric matrix
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=CMath::randn_float();
			mat(j, i)=mat(i, j);
		}
	}

	// modify one element in the matrix to destroy symmetry
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			float32_t old_val=mat(i, j);
			float32_t diff=FLT_EPSILON;

			// update, check, restore
			mat(i, j)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			// symmetric counterpart
			mat(j, i)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;
		}
	}
}

TEST(SGMatrixTest,is_symmetric_float32_false_old_minus_eps)
{
	const index_t size=2;
	SGMatrix<float32_t> mat(size, size);
	CMath::init_random(100);

	// create a symmetric matrix
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=CMath::randn_float();
			mat(j, i)=mat(i, j);
		}
	}

	// modify one element in the matrix to destroy symmetry
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			float32_t old_val=mat(i, j);
			float32_t diff=-FLT_EPSILON;

			// update, check, restore
			mat(i, j)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			// symmetric counterpart
			mat(j, i)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;
		}
	}
}

TEST(SGMatrixTest,is_symmetric_float32_true)
{
	const index_t size=2;
	SGMatrix<float32_t> mat(size, size);
	CMath::init_random(100);
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=CMath::randn_float();
			mat(j, i)=mat(i, j);
		}
	}
	EXPECT_TRUE(mat.is_symmetric());
}

TEST(SGMatrixTest,is_symmetric_float64_false_old_plus_eps)
{
	const index_t size=2;
	SGMatrix<float64_t> mat(size, size);
	CMath::init_random(100);

	// create a symmetric matrix
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=CMath::randn_double();
			mat(j, i)=mat(i, j);
		}
	}

	// modify one element in the matrix to destroy symmetry
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			float64_t old_val=mat(i, j);
			float64_t diff=DBL_EPSILON;

			// update, check, restore
			mat(i, j)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			// symmetric counterpart
			mat(j, i)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;
		}
	}
}

TEST(SGMatrixTest,is_symmetric_float64_false_old_minus_eps)
{
	const index_t size=2;
	SGMatrix<float64_t> mat(size, size);
	CMath::init_random(100);

	// create a symmetric matrix
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=CMath::randn_double();
			mat(j, i)=mat(i, j);
		}
	}

	// modify one element in the matrix to destroy symmetry
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			float64_t old_val=mat(i, j);
			float64_t diff=-DBL_EPSILON;

			// update, check, restore
			mat(i, j)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			// symmetric counterpart
			mat(j, i)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;
		}
	}
}

TEST(SGMatrixTest,is_symmetric_float64_true)
{
	const index_t size=2;
	SGMatrix<float64_t> mat(size, size);
	CMath::init_random(100);
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=CMath::randn_double();
			mat(j, i)=mat(i, j);
		}
	}
	EXPECT_TRUE(mat.is_symmetric());
}

TEST(SGMatrixTest,is_symmetric_complex128_false_old_plus_eps)
{
	const index_t size=2;
	SGMatrix<complex128_t> mat(size, size);
	CMath::init_random(100);

	// create a symmetric matrix
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=complex128_t(CMath::randn_double(), CMath::randn_double());
			mat(j, i)=mat(i, j);
		}
	}

	// modify one element in the matrix to destroy symmetry
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			complex128_t old_val=mat(i, j);
			float64_t diff=DBL_EPSILON;

			// update, check, restore
			mat(i, j)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			mat(i, j)=old_val+complex128_t(0, diff);
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			// symmetric counterpart
			mat(j, i)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;

			mat(j, i)=old_val+complex128_t(0, diff);
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;
		}
	}
}

TEST(SGMatrixTest,is_symmetric_complex128_false_old_minus_eps)
{
	const index_t size=2;
	SGMatrix<complex128_t> mat(size, size);
	CMath::init_random(100);

	// create a symmetric matrix
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=complex128_t(CMath::randn_double(), CMath::randn_double());
			mat(j, i)=mat(i, j);
		}
	}

	// modify one element in the matrix to destroy symmetry
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			complex128_t old_val=mat(i, j);
			float64_t diff=-DBL_EPSILON;

			// update, check, restore
			mat(i, j)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			mat(i, j)=old_val+complex128_t(0, diff);
			EXPECT_FALSE(mat.is_symmetric());
			mat(i, j)=old_val;

			// symmetric counterpart
			mat(j, i)=old_val+diff;
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;

			mat(j, i)=old_val+complex128_t(0, diff);
			EXPECT_FALSE(mat.is_symmetric());
			mat(j, i)=old_val;
		}
	}
}

TEST(SGMatrixTest,is_symmetric_complex128_true)
{
	const index_t size=2;
	SGMatrix<complex128_t> mat(size, size);
	CMath::init_random(100);
	for (index_t i=0; i<size; ++i)
	{
		for (index_t j=i+1; j<size; ++j)
		{
			mat(i, j)=complex128_t(CMath::randn_double(), CMath::randn_double());
			mat(j, i)=mat(i, j);
		}
	}
	EXPECT_TRUE(mat.is_symmetric());
}

TEST(SGMatrixTest, to_eigen3)
{
	const int nrows = 3;
	const int ncols = 4;

	SGMatrix<float64_t> sg_mat(nrows,ncols);
 	for (int32_t i=0; i<nrows*ncols; i++)
 		sg_mat[i] = i;

	Eigen::Map<Eigen::MatrixXd> eigen_mat = sg_mat;

	for (int32_t i=0; i<nrows*ncols; i++)
		EXPECT_EQ(sg_mat[i], eigen_mat(i));
}

TEST(SGMatrixTest, from_eigen3)
{
	const int nrows = 3;
	const int ncols = 4;

	Eigen::MatrixXd eigen_mat(nrows,ncols);
	for (int32_t i=0; i<nrows*ncols; i++)
		eigen_mat(i) = i;

	SGMatrix<float64_t> sg_mat = eigen_mat;

	for (int32_t i=0; i<nrows*ncols; i++)
		EXPECT_EQ(eigen_mat(i), sg_mat[i]);
}

TEST(SGMatrixTest, equals)
{
	const index_t size=10;
	SGMatrix<float32_t> mat, copy;

	EXPECT_TRUE(mat.equals(mat));
	EXPECT_TRUE(mat.equals(copy));

	mat=SGMatrix<float32_t>(size, size);
	CMath::init_random(100);
	for (int64_t i=0; i<mat.size(); ++i)
		mat.matrix[i]=CMath::randn_float();

	EXPECT_TRUE(mat.equals(mat));
	EXPECT_FALSE(mat.equals(copy));

	copy=SGMatrix<float32_t>(size, size);
	EXPECT_FALSE(mat.equals(copy));

	CMath::init_random(100);
	for (int64_t i=0; i<copy.size(); ++i)
		copy.matrix[i]=CMath::randn_float();

	EXPECT_TRUE(mat.equals(copy));
}

TEST(SGMatrixTest, clone)
{
	const index_t size=10;
	SGMatrix<float32_t> mat(size, size);
	for (int64_t i=0; i<mat.size(); ++i)
		mat.matrix[i]=CMath::randn_float();

	SGMatrix<float32_t> copy=mat.clone();

	EXPECT_NE(mat.data(), copy.data());
	EXPECT_TRUE(mat.equals(copy));
}

TEST(SGMatrixTest, clone_empty)
{
	SGMatrix<float32_t> mat;
	ASSERT_EQ(mat.data(), nullptr);

	SGMatrix<float32_t> copy = mat.clone();
	EXPECT_EQ(copy.data(), mat.data());
	EXPECT_TRUE(mat.equals(copy));
}

TEST(SGMatrixTest, set_const)
{
	const index_t size=10;
	SGMatrix<float64_t> mat(size, size);
	const auto value=CMath::randn_double();
	mat.set_const(value);

	for (int64_t i=0; i<mat.size(); ++i)
		EXPECT_NEAR(mat.matrix[i], value, 1E-15);
}

TEST(SGMatrixTest, max_single)
{
	const index_t size=10;
	SGMatrix<float32_t> mat(size, size);
	for (int64_t i=0; i<mat.size(); ++i)
		mat.matrix[i]=CMath::randn_float();

	auto max=mat.max_single();
	for (int64_t i=0; i<mat.size(); ++i)
		EXPECT_GE(max, mat.matrix[i]);
}

TEST(SGMatrixTest, get_slice)
{
	const index_t n_rows = 6, n_cols = 8;
	const index_t start_col = 2, end_col = 5;
	const index_t n_subcols = end_col - start_col;

	SGMatrix<float64_t> mat(n_rows, n_cols);
	for (index_t i = 0; i < n_rows * n_cols; ++i)
		mat[i] = CMath::randn_double();

	auto sub = mat.slice(start_col, end_col);

	EXPECT_EQ(sub.num_rows, mat.num_rows);
	EXPECT_EQ(sub.num_cols, end_col - start_col);
	for (index_t i = 0; i < n_rows; ++i)
		for (index_t j = 0; i < n_subcols; ++i)
			EXPECT_EQ(sub(i, j), mat(i, j + start_col));
}

TEST(SGMatrixTest, get_column)
{
	const index_t n_rows = 6, n_cols = 8;
	const index_t col = 4;

	SGMatrix<float64_t> mat(n_rows, n_cols);
	for (index_t i = 0; i < n_rows * n_cols; ++i)
		mat[i] = CMath::randn_double();

	auto vec = mat.get_column_vector(col);

	for (index_t i = 0; i < n_rows; ++i)
		EXPECT_EQ(mat(i, col), vec[i]);
}

TEST(SGMatrixTest, set_column)
{
	const index_t n_rows = 6, n_cols = 8;
	const index_t col = 4;

	SGMatrix<float64_t> mat(n_rows, n_cols);
	SGVector<float64_t> vec(n_rows);

	for (index_t i = 0; i < n_rows; ++i)
		vec[i] = CMath::randn_double();

	mat.set_column(col, vec);

	for (index_t i = 0; i < n_rows; ++i)
		EXPECT_EQ(mat(i, col), vec[i]);
}

TEST(SGMatrixTest,iterator)
{
	constexpr index_t size=5;
	SGMatrix<float64_t> mat(size, size);
	linalg::range_fill(mat, 1.0);

	auto begin = mat.begin();
	auto end = mat.end();
	EXPECT_EQ(mat.size(), std::distance(begin, end));
	EXPECT_EQ(1.0, *begin++);
	++begin;
	EXPECT_EQ(3.0, *begin);
	--begin;
	EXPECT_EQ(2.0, *begin);
	EXPECT_TRUE(begin != end);
	begin += 2;
	EXPECT_EQ(4.0, *begin);
	begin -= 1;
	EXPECT_EQ(3.0, *begin);
	EXPECT_EQ(4.0, begin[1]);
	auto new_begin = begin + 2;
	EXPECT_EQ(5.0, *new_begin);

	// range-based loop should work as well
	auto index = 0;
	for (auto v: mat)
		EXPECT_EQ(mat[index++], v);
}
