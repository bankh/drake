#include <mex.h>
#include <iostream>
#include <memory>
#include "drakeUtil.h"
#include "RigidBodyManipulator.h"
#include "drakeGeometryUtil.h"
#include "math.h"

using namespace Eigen;
using namespace std;

// TODO: stop copying these functions everywhere and find a good place for them
template <typename DerivedA>
mxArray* eigenToMatlab(const DerivedA &m)
{
 mxArray* pm = mxCreateDoubleMatrix(static_cast<int>(m.rows()),static_cast<int>(m.cols()),mxREAL);
 if (m.rows()*m.cols()>0)
   memcpy(mxGetPr(pm),m.data(),sizeof(double)*m.rows()*m.cols());
 return pm;
}

template<int RowsAtCompileTime, int ColsAtCompileTime>
Eigen::Matrix<double, RowsAtCompileTime, ColsAtCompileTime> matlabToEigen(const mxArray* matlab_array)
{
  const mwSize* size_array = mxGetDimensions(matlab_array);
  Eigen::Matrix<double, RowsAtCompileTime, ColsAtCompileTime> ret(size_array[0], size_array[1]);
  memcpy(ret.data(), mxGetPr(matlab_array), sizeof(double) * ret.size());
  return ret;
}

void mexFunction(int nlhs, mxArray *plhs[],int nrhs, const mxArray *prhs[]) {

  std::string usage = "Usage [x, J, dJ] = forwardKinVMex(model_ptr, body_or_frame_ind, points, rotation_type, base_or_frame_ind, compute_analytic_jacobian)";
  if (nrhs != 6) {
    mexErrMsgIdAndTxt("Drake:geometricJacobianmex:WrongNumberOfInputs", usage.c_str());
  }

  if (nlhs > 3) {
    mexErrMsgIdAndTxt("Drake:geometricJacobianmex:WrongNumberOfOutputs", usage.c_str());
  }

  // first get the model_ptr back from matlab
  RigidBodyManipulator *model= (RigidBodyManipulator*) getDrakeMexPointer(prhs[0]);

  int body_or_frame_ind = ((int) mxGetScalar(prhs[1])) - 1; // base 1 to base 0
  auto points = matlabToEigen<SPACE_DIMENSION, Eigen::Dynamic>(prhs[2]);
  int rotation_type = (int) mxGetScalar(prhs[3]);
  int base_or_frame_ind = ((int) mxGetScalar(prhs[4])) - 1; // base 1 to base 0
  bool compute_analytic_jacobian = (bool) (mxGetLogicals(prhs[5]))[0];

  if (compute_analytic_jacobian) {
    int gradient_order = nlhs - 1; // position gradient order
    auto x = model->forwardKinNew(points, body_or_frame_ind, base_or_frame_ind, rotation_type, gradient_order);

    plhs[0] = eigenToMatlab(x.value());
    if (gradient_order > 0)
      plhs[1] = eigenToMatlab(x.gradient().value());
    if (gradient_order > 1)
      plhs[2] = eigenToMatlab(x.gradient().gradient().value());
  }
  else {
    int gradient_order = nlhs > 1 ? nlhs - 2 : 0; // Jacobian gradient order
    auto x = model->forwardKinNew(points, body_or_frame_ind, base_or_frame_ind, rotation_type, gradient_order);
    plhs[0] = eigenToMatlab(x.value());
    if (nlhs > 1) {
      auto J = model->forwardJacV(x, body_or_frame_ind, base_or_frame_ind, rotation_type, compute_analytic_jacobian, gradient_order);
      plhs[1] = eigenToMatlab(J.value());
      if (nlhs > 2) {
        plhs[2] = eigenToMatlab(J.gradient().value());
      }
    }
  }
}
